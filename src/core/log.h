#ifndef HAMCLOCK_LOG_H
#define HAMCLOCK_LOG_H

#include <stdio.h>
#include <time.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} log_level_t;

// Initialize logging system
int log_init(const char *log_file);
void log_deinit(void);

// Set minimum log level
void log_set_level(log_level_t level);

// Core logging function
void log_msg(log_level_t level, const char *func, int line, const char *fmt, ...);

// Convenience macros
#define log_debug(fmt, ...) \
    log_msg(LOG_DEBUG, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define log_info(fmt, ...) \
    log_msg(LOG_INFO, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define log_warn(fmt, ...) \
    log_msg(LOG_WARN, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define log_error(fmt, ...) \
    log_msg(LOG_ERROR, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define log_fatal(fmt, ...) \
    log_msg(LOG_FATAL, __func__, __LINE__, fmt, ##__VA_ARGS__)

#endif // HAMCLOCK_LOG_H
