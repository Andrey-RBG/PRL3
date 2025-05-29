#ifndef LIBMYSYSLOG_H
#define LIBMYSYSLOG_H

#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3
#define CRITICAL 4

int mysyslog(const char* msg, int level, int driver, int format, const char* path);

// Путь к лог-файлу для нашего приложения
#define LOG_PATH "/var/log/myRPC.log"

// Удобные обёртки
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_fatal(const char *fmt, ...);

#endif // LIBMYSYSLOG_H
