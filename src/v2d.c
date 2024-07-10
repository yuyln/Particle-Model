#include "v2d.h"

v2d v2d_c(f64 x, f64 y) {
    return (v2d){.x = x, .y = y};
}

v2d v2d_s(f64 s) {
    return (v2d){.x = s, .y = s};
}

v2d v2d_add(v2d v1, v2d v2) {
    v1.v = _mm_add_pd(v1.v, v2.v);
    return v1;
}

v2d v2d_sub(v2d v1, v2d v2) {
    v1.v = _mm_sub_pd(v1.v, v2.v);
    return v1;
}

v2d v2d_fac(v2d v, f64 s) {
    v.v = _mm_mul_pd(v.v, (__m128d){s, s});
    return v;
}

v2d z_cross_v2d(v2d v) {
    return (v2d){.x = -v.y, .y = v.x};
}

f64 v2d_dot(v2d v1, v2d v2) {
    return v1.x * v2.x + v1.y * v2.y;
    v1.v = _mm_mul_pd(v1.v, v2.v);
    v1.v = _mm_hadd_pd(_mm_setzero_pd(), v1.v); //(0 + 0, v1.v[0] + v1.v[1])
    return v1.v[1];
}
