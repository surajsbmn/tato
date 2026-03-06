#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "logger.h"
#include "http.h"

int read_request(int socket, http_request_t *req)
{
    char buffer[MAX_REQUEST_SIZE];
    int total = 0;

    // Read until we find end of headers
    while (1)
    {
        int bytes = read(socket, buffer + total, MAX_REQUEST_SIZE - total - 1);

        if (bytes < 0)
        {
            log_error("read failed", strerror(errno));
            return -1;
        }
        if (bytes == 0)
        {
            log_error("client closed connection ", strerror(errno));
            return -1;
        }

        total += bytes;
        buffer[total] = '\0';

        if (strstr(buffer, "\r\n\r\n"))
            break;

        if (total >= MAX_REQUEST_SIZE - 1)
        {
            log_error("request too large");
            return -2;
        }
    }

    log_info("Raw Request:\n%s\n", buffer);

    // Parse request line
    char *cursor = buffer;

    char *line_end = strstr(cursor, "\r\n");
    if (!line_end)
    {
        log_error("malformed request line");
        return -1;
    }
    *line_end = '\0';

    if (sscanf(buffer, "%15s %1023s %15s", req->method, req->path, req->version) != 3)
    {
        log_error("malformed request line");
        return -1;
    }

    cursor = line_end + 2;

    req->header_count = 0;

    while (req->header_count < MAX_HEADERS)
    {
        line_end = strstr(cursor, "\r\n");
        if (!line_end)
            break;

        *line_end = '\0';

        // empty line means end of header
        if (*cursor == '\0')
            break;

        // split on ": "
        char *colon = strstr(cursor, ": ");
        if (colon)
        {
            int key_len = colon - cursor;
            int value_len = (line_end - colon) - 2;

            if (key_len > MAX_HEADER_KEY - 1)
                key_len = MAX_HEADER_KEY - 1;
            if (value_len > MAX_HEADER_VALUE - 1)
                value_len = MAX_HEADER_VALUE - 1;

            http_header_t *h = &req->headers[req->header_count++];
            strncpy(h->key, cursor, key_len);
            strncpy(h->value, colon + 2, value_len);

            h->key[key_len] = '\0';
            h->value[value_len] = '\0';
        }

        cursor = line_end + 2;
    }

    req->body = NULL;
    req->body_length = 0;

    const char *content_length_val = get_header(req, "Content-Length");
    if (content_length_val)
    {
        int content_length = atoi(content_length_val);

        if (content_length > 0 && content_length <= MAX_BODY_SIZE)
        {
            req->body = malloc(content_length + 1);
            if (!req->body)
            {
                log_error("malloc failed", strerror(errno));
                return -1;
            }
        }

        // some body bytes may already be in buffer after \r\n\r\n
        int already_read = total - (cursor - buffer);
        if (already_read > content_length)
            already_read = content_length;

        memcpy(req->body, cursor, already_read);
        req->body_length = already_read;

        // read remaining body bytes if needed
        while (req->body_length < content_length)
        {
            int bytes = read(socket,
                             req->body + req->body_length,
                             content_length - req->body_length);
            if (bytes < 0)
            {
                log_error("body read failed", strerror(errno));
                free_request(req);
                return -1;
            }
            if (bytes == 0)
                break;
            req->body_length += bytes;
        }

        req->body[req->body_length] = '\0';
    }

    return 0;
}

const char *get_header(const http_request_t *req, const char *key)
{
    for (int i = 0; i < req->header_count; i++)
    {
        if (strcmp(req->headers[i].key, key) == 0)
        {
            return req->headers[i].value;
        }
    }
    return NULL;
}

void free_request(http_request_t *req)
{
    if (req->body)
    {
        free(req->body);
        req->body = NULL;
    }
}