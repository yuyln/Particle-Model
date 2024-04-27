#include "particle_simulation.h"

#include <stdlib.h>
#include <math.h>

int main(void) {
    Table table = table_init(sin, 0, 10, 0.314);
    for (f64 x = -2; x < 15; x += 0.00314)
        printf("%f,%f,%f\n", x, table_get_value(&table, x), sin(x));
    free(table.items);
    free(table.coefs.items);
    v2d v = v2d_s(1);
    v = v2d_add(v, v);
    printf("%f %f\n", v);
    return 0;
}
