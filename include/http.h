#ifndef HTTP_H
#define HTTP_H

#define MAX_HEADERS 32
#define MAX_HEADER_KEY 64
#define MAX_HEADER_VALUE 256
#define MAX_METHOD 16
#define MAX_PATH 1024
#define MAX_VERSION 16
#define MAX_REQUEST_SIZE 8192
#define MAX_BODY_SIZE 8192

typedef struct
{
    char key[MAX_HEADER_KEY];
    char value[MAX_HEADER_VALUE];
} http_header_t;

typedef struct
{
    char method[MAX_METHOD];
    char path[MAX_PATH];
    char version[MAX_VERSION];

    http_header_t headers[MAX_HEADERS];
    int header_count;

    char *body;
    int body_length;
} http_request_t;

int read_request(int socket, http_request_t *req);
const char *get_header(const http_request_t *req, const char *key);
void free_request(http_request_t *req);

#endif