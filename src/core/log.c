#include "log.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static FILE *log_file = NULL;
static log_level_t min_level = LOG_INFO;

static const char *level_names[] = {
    "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"
};

static const char *level_colors[] = {
    "\x1b[36m",  // Cyan for DEBUG
    "\x1b[32m",  // Green for INFO
    "\x1b[33m",  // Yellow for WARN
    "\x1b[31m",  // Red for ERROR
    "\x1b[35m"   // Magenta for FATAL
};

static const char *color_reset = "\x1b[0m";

int log_init(const char *filename) {
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", filename);
            return -1;
        }
    } else {
        log_file = stderr;
    }
    return 0;
}

void log_deinit(void) {
    if (log_file && log_file != stderr) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_set_level(log_level_t level) {
    if (level >= LOG_DEBUG && level <= LOG_FATAL) {
        min_level = level;
    }
}

void log_msg(log_level_t level, const char *func, int line, const char *fmt, ...) {
    if (level < min_level) {
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    FILE *out = log_file ? log_file : stderr;

    // Check if outputting to terminal (supports color)
    // Simplified: only use color for stderr
    int use_color = (out == stderr) ? 1 : 0;

    // Print header
    if (use_color) {
        fprintf(out, "%s[%s]%s %s ", level_colors[level], timestamp, color_reset, level_names[level]);
    } else {
        fprintf(out, "[%s] %s ", timestamp, level_names[level]);
    }

    // Print function/line
    fprintf(out, "%s:%d - ", func, line);

    // Print message
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
    fflush(out);

    // Exit on FATAL
    if (level == LOG_FATAL) {
        log_deinit();
        exit(1);
    }
}
