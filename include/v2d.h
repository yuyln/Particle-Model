#ifndef __V2D_H
#define __V2D_H
#include "primitive_types.h"
#include <immintrin.h>

typedef union {
    struct {f64 x, y;};
    f64 p[2];
    __m128d v;
} v2d;

v2d v2d_c(f64 x, f64 y);
v2d v2d_s(f64 s);
v2d v2d_add(v2d v1, v2d v2);
v2d v2d_sub(v2d v1, v2d v2);
v2d v2d_fac(v2d v, f64 s);
v2d z_cross_v2d(v2d v);
f64 v2d_dot(v2d v1, v2d v2);

#endif
