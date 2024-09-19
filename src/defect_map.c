#include "defect_map.h"
#include "logging.h"
#include "utils.h"
#include "macros.h"

#include <immintrin.h>
#include <stdlib.h>

DefectMap defect_map_init(u64 rows, u64 cols, v2d limit_x, v2d limit_y, f64(*fun)(f64, f64, void*), void *user_data) {
    f64 dy = (limit_y.p[1] - limit_y.p[0]) / rows;
    f64 dx = (limit_x.p[1] - limit_x.p[0]) / cols;

    DefectMap ret = {.rows = rows, .cols = cols, .limit_x = limit_x, .limit_y = limit_y, .dx = dx, .dy = dy};

    ret.map = calloc(rows * cols * sizeof(*ret.map), 1);
    if (!ret.map)
        logging_log(LOG_FATAL, "Could not allocate %"PRIu64"x%"PRIu64" defect map values", rows, cols);

    u64 miss_to_align = 32 - (rows * cols * sizeof(*ret.coefs)) % 32;
    ret.coefs = aligned_alloc(32, rows * cols * sizeof(*ret.coefs) + miss_to_align);
    // FUCK ME: this shit needs to be 32 aligned. For some reason, using the old String everywhere (before changing to StringBuilder),
    // hid this alignment requirement. Some stack shit maybe?
    if (!ret.coefs)
        logging_log(LOG_FATAL, "Could not allocate %"PRIu64"x%"PRIu64" defect map coefs", rows, cols);

    for (u64 row = 0; row < rows; ++row) {
        f64 y = row / (f64)rows;
        y = lerp(limit_y.p[0], limit_y.p[1], y);
        for (u64 col = 0; col < cols; ++col) {
            f64 x = col / (f64)cols;
            x = lerp(limit_x.p[0], limit_x.p[1], x);
            ret.map[row * cols + col] = fun(x, y, user_data);
        }
    }
    defect_map_calculate_coefs(&ret);

    return ret;
}

void defect_map_calculate_coefs(DefectMap *it) {
    for (u64 row = 0; row < it->rows; ++row) {
        for (u64 col = 0; col < it->cols; ++col) {
            s64 right = col + 1;
            s64 left =  col - 1;
            s64 righter = col + 2;

            s64 up =    row + 1;
            s64 down =  row - 1;
            s64 upper = row + 2;

            left = ((left % (s64)it->cols) + (s64)it->cols) % (s64)it->cols;
            right = ((right % (s64)it->cols) + (s64)it->cols) % (s64)it->cols;
            righter = ((righter % (s64)it->cols) + (s64)it->cols) % (s64)it->cols;

            up = ((up % (s64)it->rows) + (s64)it->rows) % (s64)it->rows;
            down = ((down % (s64)it->rows) + (s64)it->rows) % (s64)it->rows;
            upper = ((upper % (s64)it->rows) + (s64)it->rows) % (s64)it->rows;

#ifdef BICUBIC_MAP
            //https://en.wikipedia.org/wiki/Bicubic_s32erpolation
            __m256d A[64] = {
                { 1,  0,  0,  0}, { 0,  0,  0,  0}, { 0,  0,  0,  0}, { 0,  0,  0,  0},
                { 0,  0,  0,  0}, { 1,  0,  0,  0}, { 0,  0,  0,  0}, { 0,  0,  0,  0},
                {-3,  3,  0,  0}, {-2, -1,  0,  0}, { 0,  0,  0,  0}, { 0,  0,  0,  0},
                { 2, -2,  0,  0}, { 1,  1,  0,  0}, { 0,  0,  0,  0}, { 0,  0,  0,  0},
                { 0,  0,  0,  0}, { 0,  0,  0,  0}, { 1,  0,  0,  0}, { 0,  0,  0,  0},
                { 0,  0,  0,  0}, { 0,  0,  0,  0}, { 0,  0,  0,  0}, { 1,  0,  0,  0},
                { 0,  0,  0,  0}, { 0,  0,  0,  0}, {-3,  3,  0,  0}, {-2, -1,  0,  0},
                { 0,  0,  0,  0}, { 0,  0,  0,  0}, { 2, -2,  0,  0}, { 1,  1,  0,  0},
                {-3,  0,  3,  0}, { 0,  0,  0,  0}, {-2,  0, -1,  0}, { 0,  0,  0,  0},
                { 0,  0,  0,  0}, {-3,  0,  3,  0}, { 0,  0,  0,  0}, {-2,  0, -1,  0},
                { 9, -9, -9,  9}, { 6,  3, -6, -3}, { 6, -6,  3, -3}, { 4,  2,  2,  1},
                {-6,  6,  6, -6}, {-3, -3,  3,  3}, {-4,  4, -2,  2}, {-2, -2, -1, -1},
                { 2,  0, -2,  0}, { 0,  0,  0,  0}, { 1,  0,  1,  0}, { 0,  0,  0,  0},
                { 0,  0,  0,  0}, { 2,  0, -2,  0}, { 0,  0,  0,  0}, { 1,  0,  1,  0},
                {-6,  6,  6, -6}, {-4, -2,  4,  2}, {-3,  3, -3,  3}, {-2, -1, -2, -1},
                { 4, -4, -4,  4}, { 2,  2, -2, -2}, { 2, -2,  2, -2}, { 1,  1,  1,  1} 
            };

            __m256d f = {0};       //(0, 0), (1, 0), (0, 1), (1, 1)
            __m256d dfdx = {0};    //(0, 0), (1, 0), (0, 1), (1, 1)
            __m256d dfdy = {0};    //(0, 0), (1, 0), (0, 1), (1, 1)
            __m256d d2fdxdy = {0}; //(0, 0), (1, 0), (0, 1), (1, 1)

            f[0] = it->map[row * it->cols + col];
            dfdx[0] = (it->map[row * it->cols + right] - it->map[row * it->cols + left]) / 2.0;
            dfdy[0] = (it->map[up * it->cols + col] - it->map[down * it->cols + col]) / 2.0;
            d2fdxdy[0] = (it->map[up * it->cols + right] - it->map[down * it->cols + right] - it->map[up * it->cols + left] + it->map[down * it->cols + left]) / 4.0;

            f[1] = it->map[row * it->cols + right];
            dfdx[1] = (it->map[row * it->cols + righter] - it->map[row * it->cols + col]) / 2.0;
            dfdy[1] = (it->map[up * it->cols + right] - it->map[down * it->cols + right]) / 2.0;
            d2fdxdy[1] = (it->map[up * it->cols + righter] - it->map[down * it->cols + righter] - it->map[up * it->cols + col] + it->map[down * it->cols + col]) / 4.0;

            f[2] = it->map[up * it->cols + col];
            dfdx[2] = (it->map[up * it->cols + right] - it->map[up * it->cols + left]) / 2.0;
            dfdy[2] = (it->map[upper * it->cols + col] - it->map[row * it->cols + col]) / 2.0;
            d2fdxdy[2] = (it->map[upper * it->cols + right] - it->map[row * it->cols + right] - it->map[upper * it->cols + left] + it->map[row * it->cols + left]) / 4.0;

            f[3] = it->map[up * it->cols + right];
            dfdx[3] = (it->map[up * it->cols + righter] - it->map[up * it->cols + col]) / 2.0;
            dfdy[3] = (it->map[upper * it->cols + right] - it->map[row * it->cols + right]) / 2.0;
            d2fdxdy[3] = (it->map[upper * it->cols + righter] - it->map[row * it->cols + righter] - it->map[upper * it->cols + col] + it->map[row * it->cols + col]) / 4.0;

            __m256d X[4] = {{f[0], f[1], f[2], f[3]},
                {dfdx[0], dfdx[1], dfdx[2], dfdx[3]},
                {dfdy[0], dfdy[1], dfdy[2], dfdy[3]},
                {d2fdxdy[0], d2fdxdy[1], d2fdxdy[2], d2fdxdy[3]}};

            for (int i = 0; i < 16; ++i) {
                __m256d line_m[4] = {0};
                f64 acc = 0;
                for (int j = 0; j < 4; ++j) {
                    line_m[j] = _mm256_mul_pd(A[i * 4 + j], X[j]);

                    __m256d x1 = _mm256_setzero_pd();
                    
                    // calculate 4 two-element horizontal sums:
                    // lower 64 bits contain x1[0] + x1[1]
                    // next 64 bits contain x2[0] + x2[1]
                    // next 64 bits contain x1[2] + x1[3]
                    // next 64 bits contain x2[2] + x2[3]
                    __m256d sum = _mm256_hadd_pd(x1, line_m[j]);

                    // extract upper 128 bits of result
                    __m128d sum_high = _mm256_extractf128_pd(sum, 1);

                    // add upper 128 bits of sum to its lower 128 bits
                    __m128d result = _mm_add_pd(sum_high, _mm256_extractf128_pd(sum, 0));
                    // lower 64 bits of result contain the sum of x1[0], x1[1], x1[2], x1[3]
                    // upper 64 bits of result contain the sum of x2[0], x2[1], x2[2], x2[3]

                    acc += result[1];
                }
                ((f64*)(it->coefs[row * it->cols + col].b))[i] = acc;
            }
#else
            //https://en.wikipedia.org/wiki/Bilinear_interpolation
            it->coefs[row * it->cols + col].b[0] = it->map[row * it->cols + col];
            it->coefs[row * it->cols + col].b[1] = -it->map[row * it->cols + col] + it->map[row * it->cols + right];
            it->coefs[row * it->cols + col].b[2] = -it->map[row * it->cols + col] + it->map[up * it->cols + col];
            it->coefs[row * it->cols + col].b[3] = it->map[row * it->cols + col] - it->map[up * it->cols + col] - it->map[row * it->cols + right] + it->map[up * it->cols + right];
#endif
        }
    }
}

f64 defect_map_potential_xy(f64 x, f64 y, DefectMap map) {
    x = boundary_condition_f64(x, map.limit_x);
    y = boundary_condition_f64(y, map.limit_y);

    s64 idx = (x - map.limit_x.p[0]) / (map.limit_x.p[1] - map.limit_x.p[0]) * map.cols;
    s64 idy = (y - map.limit_y.p[0]) / (map.limit_y.p[1] - map.limit_y.p[0]) * map.rows;
    MapCoeff coefs = map.coefs[idy * map.cols + idx];

    x = (x - lerp(map.limit_x.p[0], map.limit_x.p[1], idx / (f64)map.cols)) / map.dx;
    y = (y - lerp(map.limit_y.p[0], map.limit_y.p[1], idy / (f64)map.rows)) / map.dy;

#ifdef BICUBIC_MAP
    __m256d ys[4] = {{1, 1, 1, 1},
                     {y, y, y, y},
                     {y * y, y * y, y * y, y * y},
                     {y * y * y, y * y * y, y * y * y, y * y * y}};

    __m256d xs[4] = {{1, x, x * x, x * x * x},
                     {1, x, x * x, x * x * x},
                     {1, x, x * x, x * x * x},
                     {1, x, x * x, x * x * x}};

    __m256d result = {0};
    for (s32 i = 0; i < 4; ++i) {
        result = _mm256_add_pd(_mm256_mul_pd(_mm256_mul_pd(xs[i], ys[i]), coefs.b[i]), result);
    }
#else
    __m256d ys = {coefs.b[0], coefs.b[1], coefs.b[2] * y, coefs.b[3] * y}; //ys * coefs
    __m256d xs = {1, x, 1, x};
    __m256d result = _mm256_mul_pd(xs, ys);
#endif
    __m256d dummy = {0};
    __m256d sum = _mm256_hadd_pd(result, dummy); // (result[0] + result[1], dummy[0] + dummy[1], result[2] + result[3], dummy[2], dummy[3])

    //1: gets (result[0] + result[1], dummy[0] + dummy[1])
    //2: gets (result[2] + result[3], dummy[2] + dummy[3])
    //3: sums per item: (result[0..3], dummy[0..3]
    //4: returns the first component result[0..3]
     return _mm_add_pd(_mm256_extractf128_pd(sum, 0), _mm256_extractf128_pd(sum, 1))[0];
}

v2d defect_map_force_xy(f64 x, f64 y, DefectMap map) {
    x = boundary_condition_f64(x, map.limit_x);
    y = boundary_condition_f64(y, map.limit_y);

    s64 idx = (x - map.limit_x.p[0]) / (map.limit_x.p[1] - map.limit_x.p[0]) * map.cols;
    s64 idy = (y - map.limit_y.p[0]) / (map.limit_y.p[1] - map.limit_y.p[0]) * map.rows;
    MapCoeff coefs = map.coefs[idy * map.cols + idx];

    x = (x - lerp(map.limit_x.p[0], map.limit_x.p[1], idx / (f64)map.cols)) / map.dx;
    y = (y - lerp(map.limit_y.p[0], map.limit_y.p[1], idy / (f64)map.rows)) / map.dy;

    v2d ret = v2d_s(0);
    
    // d/dx
    {
#ifdef BICUBIC_MAP
        __m256d ys[4] = {{1, 1, 1, 1},
                         {y, y, y, y},
                         {y * y, y * y, y * y, y * y},
                         {y * y * y, y * y * y, y * y * y, y * y * y}};

        __m256d xs[4] = {{0, 1, 2.0 * x, 3.0 * x * x},
                         {0, 1, 2.0 * x, 3.0 * x * x},
                         {0, 1, 2.0 * x, 3.0 * x * x},
                         {0, 1, 2.0 * x, 3.0 * x * x}};

        __m256d result = {0};
        for (s32 i = 0; i < 4; ++i)
            result = _mm256_add_pd(_mm256_mul_pd(coefs.b[i], _mm256_mul_pd(xs[i], ys[i])), result);
        
#else
        __m256d ys = {1 * coefs.b[0], 1 * coefs.b[1], y * coefs.b[2], y * coefs.b[3]};
        __m256d xs = {0, 1, 0, 1};
        __m256d result = _mm256_mul_pd(xs, ys);
#endif
        __m256d dummy = {0};
        __m256d sum = _mm256_hadd_pd(result, dummy); // (result[0] + result[1], dummy[0] + dummy[1], result[2] + result[3], dummy[2], dummy[3])

        //1: gets (result[0] + result[1], dummy[0] + dummy[1])
        //2: gets (result[2] + result[3], dummy[2] + dummy[3])
        //3: sums per item: (result[0..3], dummy[0..3]
        //4: returns the first component result[0..3]
        ret.x = -_mm_add_pd(_mm256_extractf128_pd(sum, 0), _mm256_extractf128_pd(sum, 1))[0] / map.dx;
    }

    // d/dy
    {
#ifdef BICUBIC_MAP
        __m256d ys[4] = {{0, 0, 0, 0},
                          {1, 1, 1, 1},
                          {2 * y, 2 * y, 2 * y, 2 * y},
                          {3 * y * y, 3 * y * y, 3 * y * y, 3 * y * y}};

        __m256d xs[4] = {{1, x, x * x, x * x * x},
                          {1, x, x * x, x * x * x},
                          {1, x, x * x, x * x * x},
                          {1, x, x * x, x * x * x}};

        __m256d result = {0};
        for (s32 i = 0; i < 4; ++i)
            result = _mm256_add_pd(_mm256_mul_pd(coefs.b[i], _mm256_mul_pd(xs[i], ys[i])), result);
#else
        __m256d ys = {0, 0, 1, 1};
        __m256d xs = {1 * coefs.b[0], x * coefs.b[1], 1 * coefs.b[2], x * coefs.b[3]};
        __m256d result = _mm256_mul_pd(xs, ys);
#endif

        __m256d dummy = {0};
        __m256d sum = _mm256_hadd_pd(result, dummy); // (result[0] + result[1], dummy[0] + dummy[1], result[2] + result[3], dummy[2], dummy[3])

        //1: gets (result[0] + result[1], dummy[0] + dummy[1])
        //2: gets (result[2] + result[3], dummy[2] + dummy[3])
        //3: sums per item: (result[0..3], dummy[0..3]
        //4: returns the first component result[0..3]
        ret.y = -_mm_add_pd(_mm256_extractf128_pd(sum, 0), _mm256_extractf128_pd(sum, 1))[0] / map.dy;
    }

     return ret;
}

void defect_map_deinit(DefectMap *map) {
    free(map->map);
    free(map->coefs);
}
