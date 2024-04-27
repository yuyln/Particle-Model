#ifndef __TABLE_H
#define __TABLE_H
#include "macros.h"
#include "primitive_types.h"

typedef struct {
    f64 a, b, c, d;
} Coefs;

typedef struct {
    Coefs *items;
    u64 len;
    u64 cap;
} Cubics;

typedef struct {
    f64 *items;
    u64 len;
    u64 cap;

    f64 value_min;
    f64 value_max;
    f64 delta_value;

    Cubics coefs;
} Table;

Table table_init(f64(*func)(f64), f64 value_min, f64 value_max, f64 delta_value);
f64 table_get_value(const Table *table, f64 x);
Coefs coefs_from_points(f64 p0, f64 p1, f64 p2, f64 p3);

#endif
