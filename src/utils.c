#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    int code;
    const char *text;
    const char *body;
} http_status_entry_t;

static const http_status_entry_t STATUS_TABLE[] = {
    {200, "OK", "<html><body><h1>200 OK</h1></body></html>"},
    {400, "Bad Request", "<html><body><h1>400 Bad Request</h1></body></html>"},
    {403, "Forbidden", "<html><body><h1>403 Forbidden</h1></body></html>"},
    {404, "Not Found", "<html><body><h1>404 Not Found</h1></body></html>"},
    {405, "Method Not Allowed", "<html><body><h1>405 Method Not Allowed</h1></body></html>"},
    {413, "Content Too Large", "<html><body><h1>413 Content Too Large</h1></body></html>"},
    {500, "Internal Server Error", "<html><body><h1>500 Internal Server Error</h1></body></html>"},
    {501, "Not Implemented", "<html><body><h1>501 Not Implemented</h1></body></html>"},
};

#define STATUS_TABLE_LEN (sizeof(STATUS_TABLE) / sizeof(STATUS_TABLE[0]))

static const http_status_entry_t *lookup_status(int code)
{
    for (size_t i = 0; i < STATUS_TABLE_LEN; i++)
    {
        if (STATUS_TABLE[i].code == code)
            return &STATUS_TABLE[i];
    }
    return NULL;
}

char *build_error_response(int status, size_t *response_len)
{
    const http_status_entry_t *entry = lookup_status(status);

    if (!entry)
    {
        entry = lookup_status(500);
    }

    size_t body_len = strlen(entry->body);
    size_t resp_size = body_len + 256;
    char *resp = malloc(resp_size);
    if (!resp) return NULL;
    
    int len = snprintf(resp, resp_size,
                       "HTTP/1.1 %d %s\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: %zu\r\n"
                       "\r\n"
                       "%s",
                       status, entry->text, body_len, entry->body);
    
    *response_len = (size_t)len;
    return resp;
}

// returns the mime type from file path
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