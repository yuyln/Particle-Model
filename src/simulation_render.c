#include "simulation_render.h"
#include "render.h"
#include "simulation.h"
#include "logging.h"
#include "profiler.h"
#include "utils.h"

#include <stdlib.h>
#include <float.h>

u64 steps_per_frame = 10;

static void render_particles(Particles ps, DefectMap defect_map, u64 width, u64 height, RGBA32 *display) {
    for (u64 i = 0; i < ps.len; ++i) {
        v2d pos = boundary_condition(ps.items[i].pos, defect_map.limit_x, defect_map.limit_y);
        u64 ix = (pos.x - defect_map.limit_x.p[0]) / (defect_map.limit_x.p[1] - defect_map.limit_x.p[0]) * width;
        u64 iy = (pos.y - defect_map.limit_y.p[0]) / (defect_map.limit_y.p[1] - defect_map.limit_y.p[0]) * height;
        for (s64 dy = -5; dy < 5; ++dy) {
            for (s64 dx = -5; dx < 5; ++dx) {
                u64 display_x = ((((s64)ix + dx) % (s64)width) + (s64)width) % (s64)width;
                u64 display_y = ((((s64)iy + dy) % (s64)height) + (s64)height) % (s64)height;
                display_y = height - 1 - display_y;
                if (dx * dx + dy * dy < 25)
                    display[display_y * width + display_x] = (RGBA32){.a = 255};
            }
        }
    }
}

static void render_energy(BoxedParticles bp, Table particle_potential, DefectMap defect_map, u64 width, u64 height, RGBA32 *display, f64 *energy_aux) {
    f64 min_e = FLT_MAX;
    f64 max_e = -FLT_MAX;
    for (u64 iy = 0; iy < height; ++iy) {
        f64 y = iy / (f64)height * (defect_map.limit_y.p[1] - defect_map.limit_y.p[0]) + defect_map.limit_y.p[0];
        for (u64 ix = 0; ix < width; ++ix) {
            f64 x = ix / (f64)width * (defect_map.limit_x.p[1] - defect_map.limit_x.p[0]) + defect_map.limit_x.p[0];
            f64 energy = energy_at(v2d_c(x, y), bp, particle_potential, defect_map);
            max_e = energy > max_e? energy: max_e;
            min_e = energy < min_e? energy: min_e;
            energy_aux[iy * width + ix] = energy;
        }
    }

    for (u64 I = 0; I < width * height; ++I) {
        u64 ix = I % width;
        u64 iy = (I - ix) / width;
        f64 energy = energy_aux[iy * width + ix];

        energy = (energy - min_e) / (max_e - min_e);
        iy = height - 1 - iy;
        display[iy * width + ix] = (RGBA32){.r = energy * 255, .g = energy * 255, .b = energy * 255, .a = 255};

    }
}

static void render_defect_map(DefectMap defect_map, u64 width, u64 height, RGBA32 *display, f64 *energy_aux) {
    f64 min_energy = FLT_MAX;
    f64 max_energy = -FLT_MAX;
    for (u64 i = 0; i < height; ++i) {
        f64 y = i / (f64)height * (defect_map.limit_y.p[1] - defect_map.limit_y.p[0]) + defect_map.limit_y.p[0];
        for (u64 j = 0; j < width; ++j) {
            f64 x = j / (f64)width * (defect_map.limit_x.p[1] - defect_map.limit_x.p[0]) + defect_map.limit_x.p[0];
            v2d force = defect_map_force_xy(x, y, defect_map);
            f64 energy = v2d_dot(force, force);//defect_map_potential_xy(x, y, defect_map);
            max_energy = energy > max_energy? energy: max_energy;
            min_energy = energy < min_energy? energy: min_energy;
            energy_aux[i * width + j] = energy;
        }
    }

    for (u64 i = 0; i < height; ++i) {
        for (u64 j = 0; j < width; ++j) {
            f64 energy = energy_aux[i * width + j];
            energy = (energy - min_energy) / (max_energy - min_energy);
            i = height - 1 - i;
            display[i * width + j] = (RGBA32){.r = (1.0 - energy) * 255, .g = 255, .b = (1.0 - energy) * 255, .a = 255};
        }
    }
}

void simulation_render_integrate(Particles ps, Table particle_potential, DefectMap defect_map, IntegrateParams iparams, u64 width, u64 height) {
    window_init("Integrate", width, height);
    IntegrateContext ctx = integrate_context_init(ps, particle_potential, defect_map, iparams);
    RGBA32 *display = calloc(sizeof(*display) * width * height, 1);
    if (!display)
        logging_log(LOG_FATAL, "%s:%d Could not allocate memory. Buy more RAM", __FILE__, __LINE__);

    byte state = 'p';
    f64 *energy_aux = calloc(sizeof(*energy_aux) * width * height, 1);
    while (!window_should_close()) {
        profiler_start_measure("STEP_TIME");
        for (u64 t = 0; t < steps_per_frame; ++t)
            integrate_context_step(&ctx);
        profiler_end_measure("STEP_TIME");

        switch (state) {
            case 'p': 
                for (u64 i = 0; i < width * height; ++i)
                    display[i] = (RGBA32){.r = 255, .g = 255, .b = 255, .a = 255};
                render_particles(ps, defect_map, width, height, display);
                break;
            case 'e':
                render_energy(ctx.bp, particle_potential, defect_map, width, height, display, energy_aux);
                break;
            case 'd':
                render_defect_map(defect_map, width, height, display, energy_aux);
                render_particles(ps, defect_map, width, height, display);
                break;
        }

        if (window_key_pressed('p'))
            state = 'p';
        else if (window_key_pressed('e'))
            state = 'e';
        else if (window_key_pressed('d'))
            state = 'd';

        window_draw_from_bytes(display, 0, 0, width, height);
        window_render();
        window_poll();
    }
    free(display);
    integrate_context_deinit(&ctx);
    profiler_print_measures(stdout);
    free(energy_aux);
}

