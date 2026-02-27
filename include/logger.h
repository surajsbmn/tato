#ifndef LOGGER_H
#define LOGGER_H

void logger_init(const char *filename);
void logger_close();

void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_debug(const char *fmt, ...);

#endif