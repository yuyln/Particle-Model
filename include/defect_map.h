#ifndef __DEFECT_MAP_H
#define __DEFECT_MAP_H
#include "primitive_types.h"
#include "v2d.h"

#include <immintrin.h>

typedef struct {
#ifdef BICUBIC_MAP
    __m256d b[4];
#else
    __m256d b;
#endif
} MapCoeff;

typedef struct {
    f64 *map;
    MapCoeff *coefs;
    u64 rows, cols;
    v2d limit_x, limit_y;
    f64 dx, dy;
} DefectMap;

DefectMap defect_map_init(u64 rows, u64 cols, v2d limit_x, v2d limit_y, f64(*fun)(f64, f64, void*), void *user_data);
void defect_map_calculate_coefs(DefectMap *map);
f64 defect_map_potential_xy(f64 x, f64 y, DefectMap map);
v2d defect_map_force_xy(f64 x, f64 y, DefectMap map);
void defect_map_deinit(DefectMap *map);

#endif
