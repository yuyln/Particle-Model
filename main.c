#include "particle_simulation.h"

#include <stdlib.h>
#include <math.h>

int main(void) {
    Table table = table_init(sin, 0, 10, 1);
    for (f64 x = 2; x < 15; x += 0.00314)
        printf("%f,%f,%f,%f,%f\n", x, table_get_value(&table, x), table_get_derivative(&table, x), sin(x), cos(x));
    table_deinit(&table);
    return 0;
}
