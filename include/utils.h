#ifndef __UTILS_H
#define __UTILS_H
#include "v2d.h"
#include "particles.h"
#include "string_view.h"
#include "constants.h"
#include "logging.h"

#include <stdio.h>
#include <stdlib.h>

#define return_defer(value) do { ret = value; goto defer; } while(0)
#define UNUSED(x) ((void)x)
#define SIGN(x) ((x) > 0 ? 1.0: -1.0)
#define INCEPTION(M)
#define CLOSE_ENOUGH(x, y, eps) ((SIGN((x) - (y)) * ((x) - (y))) <= (eps))

#define da_append(da, item) do { \
    if ((da)->len >= (da)->cap) { \
        if ((da)->cap <= 1) \
            (da)->cap = 2;\
        else\
            (da)->cap *= 1.5; \
        (da)->items = realloc((da)->items, sizeof(*(da)->items) * (da)->cap); \
        memset(&(da)->items[(da)->len], 0, sizeof(*(da)->items) * ((da)->cap - (da)->len));\
        if (!(da)->items) \
            logging_log(LOG_FATAL, "%s:%d Could not append item to dynamic array. Allocation failed. Buy more RAM I guess, lol", __FILE__, __LINE__); \
    } \
    (da)->items[(da)->len] = (item); \
    (da)->len += 1; \
} while(0)

#ifdef DEBUG
#define debug_assert(cond) do { if (!cond) logging_log(LOG_FATAL, "%s:%d Condition \""##COND"\" failed", __FILE__, __LINE__);} while(0)
#else
#define debug_assert(cond)
#endif

v2d boundary_condition(v2d pos, v2d limit_x, v2d limit_y);
bool read_entire_file(String path, String *ret);

#endif
