#include "utils.h"
#include "macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

u64 xorshift64_u64(Xorshift64State *state) {
	u64 x = *state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return *state = x;
}

f64 xorshift64_f64(Xorshift64State *state) {
    u64 rng = xorshift64_u64(state);
    return rng / (f64)UINT64_MAX;
}

f64 normal_distribution(Xorshift64State *state) {
    for (;;) {
        f64 U = xorshift64_f64(state);
        f64 V = xorshift64_f64(state);
        f64 X = sqrt(8.0 / M_E) * (V - 0.5) / U;
        f64 X2 = X * X;
        if (X2 <= (5.0 - 4.0 * exp(0.25) * U))
            return X;
        else if (X2 >= (4.0 * exp(-1.35) / U + 1.4))
            continue;
        else if (X2 <= (-4.0 * log(U)))
            return X;
    }
}

f64 boundary_condition_f64(f64 x, v2d lim) {
    if (x < lim.p[0] || x >= lim.p[1]) {
        f64 size_x = lim.p[1] - lim.p[0];
        x = x - floor((x - lim.p[0]) / size_x) * size_x;
    }
    return x;
}

v2d boundary_condition(v2d pos, v2d limit_x, v2d limit_y) {
    pos.x = boundary_condition_f64(pos.x, limit_x);
    pos.y = boundary_condition_f64(pos.y, limit_y);
    return pos;
}

bool read_entire_file(const char *path, char **str_) {
    bool ret = true;
    FILE *f = fopen(path, "r");
    if (!f) {
        logging_log(LOG_ERROR, "Could not open file \"%s\": %s", path, strerror(errno));
        return false;
    }
    char c;
    StringBuilder str = {0};
    while ((c = fgetc(f)) != EOF)
        da_append(&str, c);
    *str_ = (char*)sb_as_cstr(&str);

    if (fclose(f) != 0) {
        logging_log(LOG_ERROR, "Could not close file \"%s\": %s", path, strerror(errno));
        ret = false;
    }

    return ret;
}

f64 lerp(f64 a, f64 b, f64 t) {
    return (b - a) * t + a;
}

char *str_fmt_tmp(const char *fmt, ...) {
    static char strs[MAX_STRS][MAX_STR_LEN] = {0};
    static uint64_t idx = 0;
    uint64_t ret_idx = idx;

    va_list arg_list;
    va_start(arg_list, fmt);
    if (!fmt) {
        logging_log(LOG_WARNING, "Format string provided is NULL");
        goto end;
    }

    if (vsnprintf(strs[idx], MAX_STR_LEN, fmt, arg_list) > MAX_STR_LEN)
        logging_log(LOG_WARNING, "String written with len greater than MAX_STR_LEN");

    va_end(arg_list);
    idx += 1;
    if (idx >= MAX_STR_LEN) {
        idx = 0;
        logging_log(LOG_WARNING, "Surpassed MAX_STRS limit. Strings are going to be overwritten, starting with %s", strs[0]);
    }
end:
    return strs[ret_idx];

}
