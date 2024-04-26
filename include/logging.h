#ifndef __LOGGING_H
#define __LOGGING_H

typedef enum {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
} logging_level;

void logging_log(logging_level level, const char *fmt, ...);
#endif

#ifdef __LOGGING_C
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "primitive_types.h"

void logging_log(logging_level level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    bool terminate = false;
    FILE *f = stdout;
    switch (level) {
        case LOG_FATAL:
            f = stderr;
            fprintf(f, "[ FATAL ] ");
            terminate = true;
            break;
        case LOG_ERROR:
            f = stderr;
            fprintf(f, "[ ERROR ] ");
            break;
        case LOG_WARNING:
            f = stderr;
            fprintf(f, "[ WARNING ] ");
            break;
        case LOG_INFO:
            fprintf(f, "[ INFO ] ");
            break;
        default: {} break;
    }
    if (fmt)
        vfprintf(f, fmt, args);

    fprintf(f, "\n");
    va_end(args);
    if (terminate)
        exit(1);
}
#endif
