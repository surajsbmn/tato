#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

static FILE *log_file = NULL;
static int log_to_stdout = 1;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void write_log(const char *level,const char *fmt, va_list args) {
    pthread_mutex_lock(&log_mutex);

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);

    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", t);

    va_list args_copy;
    va_copy(args_copy, args);

    if (log_file) {
        fprintf(log_file, "%s %s ", timebuf, level);
        vfprintf(log_file, fmt, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }

    if (log_to_stdout) {

        fprintf(stdout, "%s %s ", timebuf, level);
        vfprintf(stdout, fmt, args_copy);
        fprintf(stdout, "\n");
        fflush(stdout);

    }
    
    va_end(args_copy);

    pthread_mutex_unlock(&log_mutex);
}

void logger_init(const char *filename) {
    log_file = fopen(filename, "a");

    if(!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

void logger_close() {
    if(log_file) fclose(log_file);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_log("INFO", fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_log("ERROR", fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_log("DEBUG", fmt, args);
    va_end(args);
}
