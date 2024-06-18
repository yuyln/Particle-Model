#include "defect_map.h"
#include "logging.h"
#include "utils.h"
#include "macros.h"

#include <stdlib.h>

DefectMap defect_map_init(u64 rows, u64 cols, v2d limit_x, v2d limit_y, f64(*fun)(f64, f64, void*), void *user_data) {
    f64 dy = (limit_y.p[1] - limit_y.p[0]) / rows;
    f64 dx = (limit_x.p[1] - limit_x.p[0]) / cols;

    DefectMap ret = {.rows = rows, .cols = cols, .limit_x = limit_x, .limit_y = limit_y, .dx = dx, .dy = dy};

    ret.map = calloc(rows * cols * sizeof(*ret.map), 1);
    if (!ret.map)
        logging_log(LOG_FATAL, "Could not allocate %"PRIu64"x%"PRIu64" defect map values", rows, cols);

    ret.coefs = calloc(rows * cols * sizeof(*ret.coefs), 1);
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
            //https://en.wikipedia.org/wiki/Bicubic_s32erpolation
            f64 A[16][16] = {
                { 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                { 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                {-3,  3,  0,  0, -2, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                { 2, -2,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                { 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0},
                { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0},
                { 0,  0,  0,  0,  0,  0,  0,  0, -3,  3,  0,  0, -2, -1,  0,  0},
                { 0,  0,  0,  0,  0,  0,  0,  0,  2, -2,  0,  0,  1,  1,  0,  0},
                {-3,  0,  3,  0,  0,  0,  0,  0, -2,  0, -1,  0,  0,  0,  0,  0},
                { 0,  0,  0,  0, -3,  0,  3,  0,  0,  0,  0,  0, -2,  0, -1,  0},
                { 9, -9, -9,  9,  6,  3, -6, -3,  6, -6,  3, -3,  4,  2,  2,  1},
                {-6,  6,  6, -6, -3, -3,  3,  3, -4,  4, -2,  2, -2, -2, -1, -1},
                { 2,  0, -2,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0},
                { 0,  0,  0,  0,  2,  0, -2,  0,  0,  0,  0,  0,  1,  0,  1,  0},
                {-6,  6,  6, -6, -4, -2,  4,  2, -3,  3, -3,  3, -2, -1, -2, -1},
                { 4, -4, -4,  4,  2,  2, -2, -2,  2, -2,  2, -2,  1,  1,  1,  1} 
            };

            f64 f[4] = {0};       //(0, 0), (1, 0), (0, 1), (1, 1)
            f64 dfdx[4] = {0};    //(0, 0), (1, 0), (0, 1), (1, 1)
            f64 dfdy[4] = {0};    //(0, 0), (1, 0), (0, 1), (1, 1)
            f64 d2fdxdy[4] = {0}; //(0, 0), (1, 0), (0, 1), (1, 1)

            s32 right = col + 1;
            s32 left =  col - 1;
            s32 righter = col + 2;

            s32 up =    row + 1;
            s32 down =  row - 1;
            s32 upper = row + 2;
            
            left = ((left % it->cols) + it->cols) % it->cols;
            right = ((right % it->cols) + it->cols) % it->cols;
            righter = ((righter % it->cols) + it->cols) % it->cols;

            up = ((up % it->rows) + it->rows) % it->rows;
            down = ((down % it->rows) + it->rows) % it->rows;
            upper = ((upper % it->rows) + it->rows) % it->rows;


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

            f64 X[16] = {f[0], f[1], f[2], f[3], dfdx[0], dfdx[1], dfdx[2], dfdx[3], dfdy[0], dfdy[1], dfdy[2], dfdy[3], d2fdxdy[0], d2fdxdy[1], d2fdxdy[2], d2fdxdy[3]};

            for (s32 i = 0; i < 16; ++i) {
                it->coefs[row * it->cols + col].a[i] = 0.0;
                for (s32 j = 0; j < 16; ++j) {
                    it->coefs[row * it->cols + col].a[i] += A[i][j] * X[j];
                }
            }
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

    f64 ys[16] = {1, 1, 1, 1,
                  y, y, y, y,
                  y * y, y * y, y * y, y * y,
                  y * y * y, y * y * y, y * y * y, y * y * y};

    f64 xs[16] = {1, x, x * x, x * x * x,
                  1, x, x * x, x * x * x,
                  1, x, x * x, x * x * x,
                  1, x, x * x, x * x * x};

    f64 result = 0;
    for (s32 i = 0; i < 16; ++i)
        result += coefs.a[i] * xs[i] * ys[i];
     return result;
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
        f64 ys[16] = {1, 1, 1, 1,
                      y, y, y, y,
                      y * y, y * y, y * y, y * y,
                      y * y * y, y * y * y, y * y * y, y * y * y};

        f64 xs[16] = {0, 1, 2.0 * x, 3.0 * x * x,
                      0, 1, 2.0 * x, 3.0 * x * x,
                      0, 1, 2.0 * x, 3.0 * x * x,
                      0, 1, 2.0 * x, 3.0 * x * x};

        f64 result = 0;
        for (s32 i = 0; i < 16; ++i)
            result += coefs.a[i] * xs[i] * ys[i];
        ret.x = result / map.dx;
    }

    // d/dy
    {
        f64 ys[16] = {0, 0, 0, 0,
                      1, 1, 1, 1,
                      2 * y, 2 * y, 2 * y, 2 * y,
                      3 * y * y, 3 * y * y, 3 * y * y, 3 * y * y};

        f64 xs[16] = {1, x, x * x, x * x * x,
                      1, x, x * x, x * x * x,
                      1, x, x * x, x * x * x,
                      1, x, x * x, x * x * x};

        f64 result = 0;
        for (s32 i = 0; i < 16; ++i)
            result += coefs.a[i] * xs[i] * ys[i];
        ret.y = result / map.dy;
    }

     return v2d_fac(ret, -1);
}

void defect_map_deinit(DefectMap *map) {
    free(map->map);
    free(map->coefs);
}
