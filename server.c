#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8899

char *build_response();

int main() {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    perror("Error creating socket!");
    return 1;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  int opt = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Bind failed");
    close(sock_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(sock_fd, 3) < 0) {
    perror("Failed to listen on socket");
    close(sock_fd);
    exit(EXIT_FAILURE);
  }

  printf("Listening on port %d ...\n", PORT);
  int new_socket;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Connection handling loop
  while (1) {
    // Accept incoming connection
    if ((new_socket = accept(sock_fd, (struct sockaddr *)&client_addr,
                             &client_len)) < 0) {
      perror("Accept failed");
      close(sock_fd);
      exit(EXIT_FAILURE);
    }

    char *client_sin_addr = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);
    printf("Connection accepted from %s:%d\n", client_sin_addr, client_port);

    // read what client is sending
    char buffer[4096] = {0};
    ssize_t bytes_read = read(new_socket, buffer, sizeof(buffer) - 1);
    printf("Request:\n%s\n", buffer);

    char *response = build_response();

    send(new_socket, response, strlen(response), 0);
    free(response);
  }

  // Close the sockets
  close(new_socket);
  close(sock_fd);

  return 0;
}

char *build_response() {
  // read the source
  FILE *src_file = fopen("server.c", "r");
  char src_buf[65536] = {0};
  if (src_file) {
    fread(src_buf, 1, sizeof(src_buf) - 1, src_file);
    fclose(src_file);
  }

  // build html body
  char body[131072];
  snprintf(body, sizeof(body),
           "<!DOCTYPE html>\n"
           "<html>\n"
           "<head><title>My Web Server</title></head>\n"
           "<body>\n"
           "<h1>Hello, World!</h1>\n"
           "<h2>Here's the source code running this server:</h2>\n"
           "<plaintext>%s",
           src_buf);

  // buidl http response
  size_t response_size = strlen(body) + 256;
  char *response = malloc(response_size);
  snprintf(response, response_size,
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: %zu\r\n"
           "\r\n"
           "%s",
           strlen(body), body);
  return response;
}