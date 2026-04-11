#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>
#include "logger.h"
#include "http.h"
#include "utils.h"

#define PORT 8899
#define WEB_DIR "www"
#define LISTENER_QUEUE_SIZE 3

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void check_web_dir();
char *serve_file(http_request_t *request, size_t *response_len);

char resolved_base[PATH_MAX];

static volatile int running = 1;

// handle system signals
static void handle_signal(int sig)
{
	(void)sig;
	running = 0;
}

int main()
{
	// assign signal handler
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	logger_init("server.log");

	check_web_dir();

	// create listener socket
	int listener_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (listener_socket_fd < 0)
	{
		log_error("Error creating socket file descriptor", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Initialise socket address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// Set socket options
	int opt = 1;
	setsockopt(listener_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
	setsockopt(listener_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	// bind socket
	if (bind(listener_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		log_error("Bind failed", strerror(errno));
		close(listener_socket_fd);
		exit(EXIT_FAILURE);
	}

	// Listen on socket
	if (listen(listener_socket_fd, LISTENER_QUEUE_SIZE) < 0)
	{
		log_error("Failed to listen on socket", strerror(errno));
		close(listener_socket_fd);
		exit(EXIT_FAILURE);
	}

	log_info("Listening on port %d ...", PORT);

	int socket_fd = 0;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	// Connection handling loop
	while (running)
	{
		// Accept incoming connection
		if ((socket_fd = accept(listener_socket_fd, (struct sockaddr *)&client_addr,
								&client_len)) < 0)
		{
			if (!running)
				break;
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			log_error("Failed to accept connection", strerror(errno));
			continue;
		}

		char *client_sin_addr = inet_ntoa(client_addr.sin_addr);
		int client_port = ntohs(client_addr.sin_port);

		// read what client is sending
		http_request_t req = {0};
		int r = read_request(socket_fd, &req);
		if (r != 0)
		{
			log_error("Failed to parse request");
		}

		log_info("Request received from %s:%d %s %s",
				 client_sin_addr, client_port, req.method, req.path);
		// Serve html files from public dir
		size_t response_len;
		char *response = serve_file(&req, &response_len);

		send(socket_fd, response, response_len, 0);
		free(response);
		close(socket_fd);
	}

	// Close the sockets
	log_info("Shutting down...");
	close(socket_fd);
	close(listener_socket_fd);

	logger_close();

	return 0;
}

char *serve_file(http_request_t *request, size_t *out_len)
{
	// #TODO  add consts
	if (strcmp(request->method, "GET") != 0)
	{
		return build_error_response(405, out_len);
	}

	// if get then read request->path
	char filepath[PATH_MAX];
	char *request_path = request->path;
	if (strcmp(request_path, "/") == 0)
	{
		request_path = "/index.html";
	}

	snprintf(filepath, sizeof(filepath), "%s%s", WEB_DIR, request_path);
	char resolved_path[PATH_MAX];
	if (realpath(filepath, resolved_path) == NULL)
	{
		return build_error_response(404, out_len);
	}

	if (strncmp(resolved_path, resolved_base, strlen(resolved_base)) != 0)
	{
		return build_error_response(403, out_len);
	}

	// if file exists hen return the html file
	struct stat st;
	if (stat(resolved_path, &st) < 0)
	{
		return build_error_response(404, out_len);
	}

	if (!S_ISREG(st.st_mode))
	{
		return build_error_response(403, out_len);
	}

	FILE *f = fopen(resolved_path, "rb");
	if (!f)
	{
		return build_error_response(500, out_len);
	}

	char *file_buf = malloc(st.st_size);
	fread(file_buf, 1, st.st_size, f);
	fclose(f);

	const char *mime = mime_from_path(resolved_path);

	char headers[256];
	int headers_len = snprintf(headers, sizeof(headers),
							   "HTTP/1.1 200 OK\r\n"
							   "Content-Type: %s\r\n"
							   "Content-Length: %ld\r\n"
							   "\r\n",
							   mime, st.st_size);
	char *response = malloc(headers_len + st.st_size);
	memcpy(response, headers, headers_len);
	memcpy(response + headers_len, file_buf, st.st_size);

	free(file_buf);

	*out_len = headers_len + st.st_size;

	return response;
}

void check_web_dir()
{
	if (realpath(WEB_DIR, resolved_base) == NULL)
	{
		log_error("Web dir not found");
		exit(1);
	}
}
