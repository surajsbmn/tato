#ifndef UTILS_H
#define UTILS_H

char *build_error_response(int status, size_t *response_len);
const char *mime_from_path(const char *path);

#endif