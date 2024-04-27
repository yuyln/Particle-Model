#include "table.h"
#include "logging.h"

#include <string.h>
#include <stdlib.h>

Table table_init(f64(*func)(f64), f64 xmin, f64 xmax, f64 dx) {
    Table ret = {.value_max = xmax, .value_min = xmin, .delta_value = dx};
    for (f64 x = xmin; x <= xmax; x += dx) {
        da_append(&ret, func(x));
        da_append(&ret.coefs, coefs_from_points(func(x + 0.0 / 3.0 * dx), func(x + 1.0 / 3.0 * dx), func(x + 2.0 / 3.0 * dx), func(x + 3.0 / 3.0 * dx)));
    }
    return ret;
}

f64 table_get_value(const Table *table, f64 x) {
    s64 idx = (x - table->value_min) / table->delta_value;
    x -= table->value_min;
    if (idx < 0)
        return table->items[0];
    else if ((u64)idx >= table->len)
        return table->items[table->len - 1];
    x = (x - idx * table->delta_value) / table->delta_value;
    Coefs coef = table->coefs.items[idx];
    return coef.a * x * x * x + coef.b * x * x + coef.c * x + coef.d;
}

f64 table_get_derivative(const Table *table, f64 x) {
    s64 idx = (x - table->value_min) / table->delta_value;
    x -= table->value_min;
    if (idx < 0)
        return table_get_derivative(table, table->value_min);
    else if ((u64)idx >= table->len)
        return table_get_derivative(table, table->value_max);
    x = (x - idx * table->delta_value) / table->delta_value;
    Coefs coef = table->coefs.items[idx];
    f64 ret = (3.0 * coef.a * x * x + 2.0 * coef.b * x + coef.c) / table->delta_value; //What the actual fuck?
    return ret;
}

Coefs coefs_from_points(f64 p0, f64 p1, f64 p2, f64 p3) {
    return (Coefs) {
        .a = (9.0 * p3 - 27.0 * p2 + 27.0 * p1 - 9.0 * p0) / 2.0,
        .b = -((9.0 * p3 - 36.0 * p2 + 45.0 * p1 - 18.0 * p0) / 2.0),
        .c = (2.0 * p3 - 9.0 * p2 + 18.0 * p1 - 11.0 * p0) / 2.0,
        .d = p0
    };
}

void table_deinit(Table *table) {
    free(table->items);
    free(table->coefs.items);
    memset(table, 0, sizeof(*table));
}
