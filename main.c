#include "particle_simulation.h"

#include <stdlib.h>
#include <math.h>

f64 test(f64 x, f64 y, void *dummy) {
    return 0.5 * (sin(2.0 * M_PI * 10 * x) + sin(2.0 * M_PI * 10 * y));
}

#define WIDTH 600
#define HEIGHT 600

int main(void) {
    f64 max_x = 1;
    f64 max_y = 1;
    DefectMap defect_map = defect_map_init(200, 200, v2d_c(0, max_x), v2d_c(0, max_y), test, NULL);
    RGBA32 *defect_map_rgb = calloc(WIDTH * HEIGHT * sizeof(*defect_map_rgb), 1);

    window_init("Defect Map", WIDTH, HEIGHT);

    for (u64 idy = 0; idy < HEIGHT; ++idy) {
        f64 y = idy / (f64)HEIGHT * max_y;
        for (u64 idx = 0; idx < WIDTH; ++idx) {
            f64 x = idx / (f64)WIDTH * max_x;
            v2d f = defect_map_force_xy(x, y, defect_map);

            f64 val = 0.5 * (defect_map_potential_xy(x, y, defect_map) + 1);
            f.x = (f.x + M_PI * 10) / (2.0 * M_PI * 10);
            f.y = (f.y + M_PI * 10) / (2.0 * M_PI * 10);

            if (val > 1 || f.x > 1 || f.y > 1)
                logging_log(LOG_INFO, "val: %f - f: %f %f", val, f.x, f.y);

            defect_map_rgb[idy * WIDTH + idx] = (RGBA32){.r = 240 * f.x, .g = 240 * f.y, .b = 0, .a = 255};
        }
    }

    while (!window_should_close()) {
        window_draw_from_bytes(defect_map_rgb, 0, 0, WIDTH, HEIGHT);
        window_render();
        window_poll();
    }

    defect_map_deinit(&defect_map);
    free(defect_map_rgb);
    return 0;
}
