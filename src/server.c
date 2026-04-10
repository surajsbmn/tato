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

#define PORT 8899
#define WEB_DIR "www"
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void check_web_dir();
char *serve_file(http_request_t *request, size_t *response_len);
char *build_error_response(int status, const char *status_text, const char *body, size_t *out_len);
const char *mime_from_path(const char *path);

char resolved_base[PATH_MAX];

static volatile int running  = 1;

static void handle_signal(int sig) 
{
	(void)sig;
	running = 0;
}

int main()
{
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	logger_init("server.log");

	check_web_dir();
	int listener_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (listener_socket < 0)
	{
		log_error("Error creating socket file descriptor", strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	int opt = 1;
	setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
	setsockopt(listener_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (bind(listener_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		log_error("Bind failed", strerror(errno));
		close(listener_socket);
		exit(EXIT_FAILURE);
	}

	if (listen(listener_socket, 3) < 0)
	{
		log_error("Failed to listen on socket", strerror(errno));
		close(listener_socket);
		exit(EXIT_FAILURE);
	}

	log_info("Listening on port %d ...", PORT);

	int socket = 0;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	// Connection handling loop
	while (running)
	{
		// Accept incoming connection
		if ((socket = accept(listener_socket, (struct sockaddr *)&client_addr,
							 &client_len)) < 0)
		{
			if(!running) break;
			if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
			log_error("Failed to accept connection", strerror(errno));
			continue;			
		}

		char *client_sin_addr = inet_ntoa(client_addr.sin_addr);
		int client_port = ntohs(client_addr.sin_port);
		log_info("Connection accepted from %s:%d\n", client_sin_addr, client_port);

		// read what client is sending
		http_request_t req = {0};
		int r = read_request(socket, &req);
		if (r != 0)
		{
			log_error("Failed to parse request");
		}
		// Serve html files from public dir
		size_t response_len;
		char *response = serve_file(&req, &response_len);

		send(socket, response, response_len, 0);
		free(response);
		close(socket);
	}

	// Close the sockets
	log_info("Shutting down...");
	close(socket);
	close(listener_socket);
	logger_close();
	return 0;
}

char *serve_file(http_request_t *request, size_t *out_len)
{
	// #TODO  add consts
	if (strcmp(request->method, "GET") != 0)
	{
		return build_error_response(405, "Method Not Allowed", "<h1>405 Method Not Allowed</h1>", out_len);
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
		return build_error_response(404, "Not Found", "<h1>404 Not Found</h1>", out_len);
	}

	if (strncmp(resolved_path, resolved_base, strlen(resolved_base)) != 0)
	{
		return build_error_response(403, "Forbidden", "<h1>403 Forbidden</h1>", out_len);
	}

	// if file exists hen return the html file
	struct stat st;
	if (stat(resolved_path, &st) < 0)
	{
		return build_error_response(404, "Not Found", "<h1>404 Not Found</h1>", out_len);
	}

	if (!S_ISREG(st.st_mode))
	{
		return build_error_response(403, "Forbidden", "<h1>403 Forbidden</h1>", out_len);
	}

	FILE *f = fopen(resolved_path, "rb");
	if (!f)
	{
		return build_error_response(500, "Internal Server Error", "<h1>500 Internal Server Error</h1>", out_len);
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

char *build_error_response(int status, const char *status_text, const char *body, size_t *out_len)
{
	size_t body_len = strlen(body);
	size_t resp_size = body_len + 256;
	char *resp = malloc(resp_size);
	snprintf(resp, resp_size,
			 "HTTP/1.1 %d %s\r\n"
			 "Content-Type: text/html\r\n"
			 "Content-Length: %zu\r\n"
			 "\r\n"
			 "%s",
			 status, status_text, body_len, body);
	*out_len = strlen(resp);
	return resp;
}

const char *mime_from_path(const char *path)
{
	const char *ext = strrchr(path, '.');
	if (!ext)
		return "application/octet-stream";
	if (strcmp(ext, ".html") == 0)
		return "text/html";
	if (strcmp(ext, ".css") == 0)
		return "text/css";
	if (strcmp(ext, ".js") == 0)
		return "application/javascript";
	if (strcmp(ext, ".png") == 0)
		return "image/png";
	if (strcmp(ext, ".jpg") == 0)
		return "image/jpeg";
	if (strcmp(ext, ".ico") == 0)
		return "image/x-icon";
	return "application/octet-stream";
}