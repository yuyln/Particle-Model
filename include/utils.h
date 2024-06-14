#ifndef __UTILS_H
#define __UTILS_H
#include "v2d.h"
#include "string_view.h"
#include "logging.h"
#include "v2d.h"

typedef struct {
    v2d start;
    v2d size;
} Box;

f64 boundary_condition_f64(f64 x, v2d lim);
v2d boundary_condition(v2d pos, v2d limit_x, v2d limit_y);
bool read_entire_file(String path, String *ret);
f64 lerp(f64 a, f64 b, f64 t);

#endif
