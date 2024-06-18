#ifndef __INTEGRATE_H
#define __INTEGRATE_H

#include "defect_map.h"
#include "primitive_types.h"
#include "boxed_particles.h"
#include "table.h"

#include <stdio.h>

typedef struct {
    f64 dt;
    f64 total_time;
    u64 interval_for_information;
    const char *path_for_information;
    f64 max_derivative;

    v2d(*drive_function)(f64, v2d, void *);
    f64(*temperature_function)(f64, v2d, void *);
    void *drive_data;
    void *temperature_data;
} IntegrateParams;

typedef struct {
    Particles ps0;
    IntegrateParams params;
    BoxedParticles bp;
    Table potential;
    DefectMap defects;
    f64 time;
    u64 step;
    u64 expected_steps;
    FILE *information_file;

    v2d *avg_vel;
    v2d *inst_vel;
} IntegrateContext;

IntegrateParams integrate_params_init();
IntegrateContext integrate_context_init(Particles ps, Table particle_potential, DefectMap defects, IntegrateParams params);
void integrate_context_deinit(IntegrateContext *ctx);
void integrate_context_step(IntegrateContext *ctx);
void integrate_context_print_info(IntegrateContext ctx);
v2d integrate(Particles ps, Table particle_potential, DefectMap defects, IntegrateParams params);

#endif
