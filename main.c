#include "particle_simulation.h"
#include "table.h"
#include "bessel.h"

#include <float.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

f64 dra() {
    return rand() / (f64)RAND_MAX;
}

static const u64 n = 5;
f64 kx = 2.0 * M_PI * n / 36.0;
f64 ky = 2.0 * M_PI * n / 36.0;
f64 test(f64 x, f64 y, void *dummy) {
    return (cos(kx * x) + cos(ky * y)) * 2;
}

v2d drive(f64 t, v2d xy, void *d) {
    f64 c = *(f64*)d;
    return z_cross_v2d(v2d_c(c, 0));
}

f64 temp(f64 t, v2d xy, void *d) {
    return 0.06;
}

#define WIDTH 900
#define HEIGHT 900

int main(void) {
    window_init("Defect Map", WIDTH, HEIGHT);
    RGBA32 *display = calloc(WIDTH * HEIGHT * sizeof(*display), 1);

    f64 lx = 36;
    f64 ly = lx;

    v2d sx = v2d_c(0, lx);
    v2d sy = v2d_c(0, ly);

    DefectMap defect_map = defect_map_init(2000, 2000, sx, sy, test, NULL);
    Particles ps = {0};
    Particles ps0 = {0};

    for (u64 i = 1; i < 2 * n; i += 2)
        for (u64 j = 1; j < 2 * n; j += 2)
            da_append(&ps, ((Particle){.pos = v2d_c(j * M_PI / kx, i * M_PI / ky), .magnus = sin(M_PI / 4.0), .damping = cos(M_PI / 4.0), .u0 = 1}));

    {
        s32 yc0 = M_PI * 5 / ky;
        s32 xc0 = M_PI * 5 / kx;

        s32 yc1 = M_PI * 7 / ky;
        s32 xc1 = M_PI * 7 / kx;

        s32 xc = (xc0 + xc1) / 2;
        s32 yc = (yc0 + yc1) / 2;

        da_append(&ps, ((Particle){.pos = v2d_c(xc, yc), .magnus = sin(26 * M_PI / 180.0), .damping = cos(26 * M_PI / 180.0), .u0 = 1}));
    }


    for (u64 i = 0; i < ps.len; ++i)
        da_append(&ps0, ps.items[i]);

    BoxedParticles bp = boxed_particles_init(5, 5, sx, sy, ps);
    boxed_particles_update(&bp);
    Table table = table_init(bessk0, EPS, 10, 0.001);

    u64 counter = 0;
    byte state = 'p';
    f64 dt = 0.01;
    f64 time = 0;
    f64 current = 0;
    while (!window_should_close()) {
        for (u64 t = 0; t < 1; ++t) {
#pragma omp parallel for num_threads(5)
            for (u64 i = 0; i < ps.len; ++i) {
                v2d force = force_at_particle_rk4(time, dt, i, ps.items[i], bp, table, defect_map, drive, &current, temp, NULL);
                ps0.items[i].pos = v2d_add(bp.ps.items[i].pos, force);
                ps0.items[i].pos = boundary_condition(ps0.items[i].pos, sx, sy);
            }

#pragma omp parallel for num_threads(5)
            for (u64 i = 0; i < ps.len; ++i) {
                ps.items[i].pos = ps0.items[i].pos;
            }
            boxed_particles_update(&bp);
            time += dt;
            if (time > 20) {
                current += 0.1;
                printf("%f\n", current);
                time = 0;

                u64 counter = 0;
                for (u64 i = 1; i < 2 * n; i += 2)
                    for (u64 j = 1; j < 2 * n; j += 2)
                        ps.items[counter++].pos = v2d_c(j * M_PI / kx, i * M_PI / ky);
                {
                    s32 yc0 = M_PI * 5 / ky;
                    s32 xc0 = M_PI * 5 / kx;

                    s32 yc1 = M_PI * 7 / ky;
                    s32 xc1 = M_PI * 7 / kx;

                    s32 xc = (xc0 + xc1) / 2;
                    s32 yc = (yc0 + yc1) / 2;

                    ps.items[counter].pos = v2d_c(xc, yc);
                }


                for (u64 i = 0; i < ps.len; ++i)
                    ps0.items[i] = ps.items[i];
            }
        }

        switch (state) {
            case 'p': {
                for (u64 i = 0; i < WIDTH * HEIGHT; ++i)
                    display[i] = (RGBA32){.r = 255, .g = 255, .b = 255, .a = 255};

                for (u64 i = 0; i < ps.len; ++i) {
                    u64 ix = (bp.ps.items[i].pos.x - sx.p[0]) / (sx.p[1] - sx.p[0]) * WIDTH;
                    u64 iy = (bp.ps.items[i].pos.y - sy.p[0]) / (sy.p[1] - sy.p[0]) * HEIGHT;
                    for (s64 dy = -5; dy < 5; ++dy) {
                        for (s64 dx = -5; dx < 5; ++dx) {
                            u64 display_x = ((((s64)ix + dx) % WIDTH) + WIDTH) % WIDTH;
                            u64 display_y = ((((s64)iy + dy) % HEIGHT) + HEIGHT) % HEIGHT;
                            display_y = HEIGHT - 1 - display_y;
                            if (dx * dx + dy * dy < 25)
                                display[display_y * WIDTH + display_x] = (RGBA32){.a = 255};
                        }
                    }
                }
                      }
                break;
            case 'e': {
                f64 min_e = FLT_MAX;
                f64 max_e = -FLT_MAX;
                for (u64 iy = 0; iy < HEIGHT; ++iy) {
                    f64 y = iy / (f64)HEIGHT * (sy.p[1] - sy.p[0]) + sy.p[0];
                    for (u64 ix = 0; ix < WIDTH; ++ix) {
                        f64 x = ix / (f64)WIDTH * (sx.p[1] - sx.p[0]) + sx.p[0];
                        v2d force = defect_map_force_xy(x, y, defect_map);
                        max_e = force.x > max_e? force.x: max_e;
                        max_e = force.y > max_e? force.y: max_e;
                        min_e = force.x < min_e? force.x: min_e;
                        min_e = force.y < min_e? force.y: min_e;
                    }
                }

#pragma omp parallel for num_threads(5)
                for (u64 I = 0; I < WIDTH * HEIGHT; ++I) {
                    u64 ix = I % WIDTH;
                    u64 iy = (I - ix) / WIDTH;
                    f64 y = iy / (f64)HEIGHT * (sy.p[1] - sy.p[0]) + sy.p[0];
                    f64 x = ix / (f64)WIDTH * (sx.p[1] - sx.p[0]) + sx.p[0];
                    iy = HEIGHT - 1 - iy;
                    v2d force = defect_map_force_xy(x, y, defect_map);

                    force = v2d_sub(force, v2d_s(min_e));
                    force = v2d_fac(force, 1.0 / (max_e - min_e));
                    display[iy * WIDTH + ix] = (RGBA32){.r = force.x * 255, .g = force.y * 255, .b = 0 * 255, .a = 255};

                }
            }
                break;
        }

        if (window_key_pressed('p'))
            state = 'p';
        else if (window_key_pressed('e'))
            state = 'e';

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
