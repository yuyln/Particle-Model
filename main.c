#include "particle_simulation.h"
#include "bessel.h"

#include <stdlib.h>
#include <math.h>

int main(void) {
    Table table = table_init(bessk0, EPS, 10, 0.0001);
    for (f64 x = EPS; x < 15; x += 0.00001)
        printf("%f,%f,%f,%f,%f\n", x, table_get_value(&table, x), table_get_derivative(&table, x), bessk0(x), -bessk1(x));
    table_deinit(&table);
    return 0;
}
