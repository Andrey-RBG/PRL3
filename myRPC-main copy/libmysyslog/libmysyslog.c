#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "libmysyslog.h"

int mysyslog(const char* msg, int level, int driver, int format, const char* path) {
    FILE *log_file = fopen(path, "a");
    if (!log_file) return -1;

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char *level_str = "UNKNOWN";
    switch(level) {
        case DEBUG: level_str = "DEBUG"; break;
        case INFO: level_str = "INFO"; break;
        case WARN: level_str = "WARN"; break;
        case ERROR: level_str = "ERROR"; break;
        case CRITICAL: level_str = "CRITICAL"; break;
    }

    if (format == 0) {
        fprintf(log_file, "%s %s %d %s\n", timestamp, level_str, driver, msg);
    } else {
        fprintf(log_file, "{\"timestamp\":\"%s\",\"log_level\":\"%s\",\"driver\":%d,\"message\":\"%s\"}\n",
                timestamp, level_str, driver, msg);
    }
    fclose(log_file);
    return 0;
}

// Реализация обёрток

static void log_va(int level, const char *fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    mysyslog(buf, level, 0, 0, LOG_PATH);
}

void log_debug(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_va(DEBUG, fmt, ap);
    va_end(ap);
}

void log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_va(INFO, fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_va(WARN, fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_va(ERROR, fmt, ap);
    va_end(ap);
}

void log_fatal(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_va(CRITICAL, fmt, ap);
    va_end(ap);
}
