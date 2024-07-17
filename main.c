#include "particle_simulation.h"

#include <stdlib.h>
#include <math.h>

static const f64 lx = 36;
static const f64 ly = lx;

static const v2d sx = (v2d){.p[0] = 0, .p[1] = lx};
static const v2d sy = (v2d){.p[0] = 0, .p[1] = ly};

static const u64 n = 5;
f64 kx = 2.0 * M_PI * n / lx;
f64 ky = 2.0 * M_PI * n / ly;

f64 moire_lattice(f64 x, f64 y, void *unused) {
    UNUSED(unused);
    return (cos(kx * x) + cos(ky * y)) * 2;
}

v2d drive(f64 t, v2d xy, void *d) {
    UNUSED(t);
    UNUSED(xy);
    f64 c = *(f64*)d;
    return z_cross_v2d(v2d_c(0, c));
}

f64 temperature(f64 t, v2d xy, void *d) {
    UNUSED(t);
    UNUSED(xy);
    UNUSED(d);
    return 0.00;
}

f64 particle_potential(f64 distance, void *unused) {
    UNUSED(unused);
    return bessk0(distance);
}

//@TODO: SIMD: DefectMap, v2d and Table
//@TODO: Avoid jumping around in memory when using inner boxes
int main(void) {
    Particles ps = {0};

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

        da_append(&ps, ((Particle){.pos = v2d_c(xc, yc), .magnus = ps.items[ps.len - 1].magnus, .damping = ps.items[ps.len - 1].damping, .u0 = ps.items[ps.len - 1].u0}));
    }

    Table table = table_init(particle_potential, EPS, 10, 0.001, NULL);
    DefectMap defect_map = defect_map_init(2000, 2000, sx, sy, moire_lattice, NULL);

    IntegrateParams iparams = integrate_params_init();
    iparams.drive_function = drive;
    iparams.temperature_function = temperature;
    iparams.interval_for_information = 0;
    iparams.dt = 0.01;
    iparams.total_time = iparams.dt * 3e4;

    FILE *data_per_current = fopen("data.dat", "w");
    if (!data_per_current)
        logging_log(LOG_FATAL, "Could not open file for writing velocities versus drive");
    if (fprintf(data_per_current, "Fd,Vx,Vy,R,Theta\n") < 0)
        logging_log(LOG_FATAL, "Could not write on `data_per_current` file");

    f64 current = 1.5;
        fflush(data_per_current);
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

            ps.items[counter++].pos = v2d_c(xc, yc);
        }
        iparams.drive_data = &current;
        //v2d avg_vel = integrate(ps, table, defect_map, iparams);
        //if (fprintf(data_per_current, "%.15e,%.15e,%.15e,%.15e,%.15e\n", current, avg_vel.x, avg_vel.y, avg_vel.y / avg_vel.x, atan2(avg_vel.y, avg_vel.x) * 180.0 / M_PI) < 0)
        //    logging_log(LOG_FATAL, "Could not write on `data_per_current` file");
        integrate(ps, table, defect_map, iparams);
    fclose(data_per_current);
    free(ps.items);
    defect_map_deinit(&defect_map);
    table_deinit(&table);
    return 0;
}
