#include "particle_simulation.h"
#include "table.h"
#include "bessel.h"

#include <stdlib.h>
#include <math.h>

f64 dra() {
    return rand() / (f64)RAND_MAX;
}

f64 test(f64 x, f64 y, void *dummy) {
    return 0;
    return 0.5 * (sin(2.0 * M_PI * 10 * x) + sin(2.0 * M_PI * 10 * y));
}

#define WIDTH 600
#define HEIGHT 600

int main(void) {
    window_init("Defect Map", WIDTH, HEIGHT);
    RGBA32 *display = calloc(WIDTH * HEIGHT * sizeof(*display), 1);

    f64 lx = 10;
    f64 ly = lx * sqrt(3.0) / 2.0;

    v2d sx = v2d_c(0, lx);
    v2d sy = v2d_c(0, ly);

    DefectMap defect_map = defect_map_init(200, 200, sx, sy, test, NULL);
    Particles ps = {0};
    Particles ps0 = {0};

    for (u64 i = 0; i < 81; ++i)
        da_append(&ps, ((Particle){.pos = v2d_c(lx * dra(), ly * dra()), .magnus_ratio = 0}));
    
    for (u64 i = 0; i < ps.len; ++i)
        da_append(&ps0, ps.items[i]);

    BoxedParticles bp = boxed_particles_init(1, 1, sx, sy, ps);
    boxed_particles_update(&bp);
    Table table = table_init(bessk0, EPS, 30, 50000);

    u64 counter = 0;
    while (!window_should_close()) {
        for (u64 t = 0; t < 1; ++t) {
            for (u64 i = 0; i < ps.len; ++i) {
                v2d force = force_at_xy(bp.ps.items[i].pos, bp, table, defect_map);
                ps0.items[i].pos = v2d_add(bp.ps.items[i].pos, v2d_fac(force, 0.01));
                ps0.items[i].pos = boundary_condition(ps0.items[i].pos, sx, sy);
            }

            for (u64 i = 0; i < ps.len; ++i) {
                ps.items[i].pos = ps0.items[i].pos;
            }
            boxed_particles_update(&bp);
        }


        for (u64 i = 0; i < WIDTH * HEIGHT; ++i)
            display[i] = (RGBA32){.r = 255, .g = 255, .b = 255, .a = 255};
        for (u64 i = 0; i < ps.len; ++i) {
            u64 display_x = (bp.ps.items[i].pos.x - sx.p[0]) / (sx.p[1] - sx.p[0]) * WIDTH;
            u64 display_y = (bp.ps.items[i].pos.y - sy.p[0]) / (sy.p[1] - sy.p[0]) * HEIGHT;
            for (s64 dy = -5; dy < 5; ++dy)
                for (s64 dx = -5; dx < 5; ++dx)
                    if ((display_y + dy) >= 0 && (display_y + dy) < HEIGHT && (display_x + dx) >= 0 && (display_x + dx) < WIDTH)
                        display[(display_y + dy) * WIDTH + (display_x + dx)] = (RGBA32){.r = 0, .g = 0, .b = 0, .a = 255};
        }
        window_draw_from_bytes(display, 0, 0, WIDTH, HEIGHT);
        window_render();
        window_poll();
        counter += 1;
    }

    defect_map_deinit(&defect_map);
    free(display);
    boxed_particles_deinit(&bp);
    free(ps.items);
    table_deinit(&table);
    free(ps0.items);
    return 0;
}
