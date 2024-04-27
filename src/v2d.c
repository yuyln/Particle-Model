#include "v2d.h"

v2d v2d_c(f64 x, f64 y) {
    return (v2d){.x = x, .y = y};
}

v2d v2d_s(f64 s) {
    return (v2d){.x = s, .y = s};
}

v2d v2d_add(v2d v1, v2d v2) {
    v1.x += v2.x;
    v1.y += v2.y;
    return v1;
}

v2d v2d_sub(v2d v1, v2d v2) {
    v1.x -= v2.x;
    v1.y -= v2.y;
    return v1;
}

v2d v2d_fac(v2d v, f64 s) {
    v.x *= s;
    v.y *= s;
    return v;
}

v2d z_cross_v2d(v2d v) {
    return (v2d){.x = -v.y, .y = v.x};
}

f64 v2d_dot(v2d v1, v2d v2) {
    return v1.x * v2.x + v1.y * v2.y;
}
