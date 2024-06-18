#include "integrate.h"
#include "logging.h"
#include "macros.h"
#include "simulation.h"
#include "utils.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

IntegrateParams integrate_params_init() {
    IntegrateParams ret = {0};
    ret.dt = 0.001;
    ret.total_time = 3e6 * ret.dt;
    ret.path_for_information = "integrate_info.csv";
    ret.interval_for_information = 1000;
    ret.max_derivative = 1.0e-4;
    return ret;
}

IntegrateContext integrate_context_init(Particles ps, Table particle_potential, DefectMap defects, IntegrateParams params) {
    IntegrateContext ctx = {0};
    ctx.params = params;

    for (u64 i = 0; i < ps.len; ++i)
        da_append(&ctx.ps0, ps.items[i]);

    f64 cut = table_get_cut(particle_potential, params.max_derivative);

    logging_log(LOG_INFO, "Particle potential cut found: %.15e. Potential(cut) = %.15e | Derivative(cut) = %.15e", cut, table_get_value(particle_potential, cut), table_get_derivative(particle_potential, cut));

    u64 rows = (defects.limit_y.p[1] - defects.limit_y.p[0]) / cut;
    u64 cols = (defects.limit_x.p[1] - defects.limit_x.p[0]) / cut;

    ctx.bp = boxed_particles_init(rows, cols, defects.limit_x, defects.limit_y, ps);
    ctx.potential = particle_potential;
    ctx.defects = defects;
    ctx.time = 0;
    ctx.step = 0;

    ctx.information_file = fopen(params.path_for_information, "w");
    if (!ctx.information_file)
        logging_log(LOG_FATAL, "%s:%d Could not open file \"%s\": %s", __FILE__, __LINE__, params.path_for_information, strerror(errno));
    if (fprintf(ctx.information_file, "t,") < 0)
        logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);

    for (u64 i = 0; i < ps.len; ++i)
        if (fprintf(ctx.information_file, "x%"PRIu64",y%"PRIu64",", i, i) < 0)
            logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);

    for (u64 i = 0; i < ps.len - 1; ++i)
        if (fprintf(ctx.information_file, "vx%"PRIu64",vy%"PRIu64",", i, i) < 0)
            logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);
    {
        u64 i = ps.len - 1;
        if (fprintf(ctx.information_file, "vx%"PRIu64",vy%"PRIu64"\n", i, i) < 0)
            logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);
    }

    ctx.avg_vel = calloc(sizeof(*ctx.avg_vel) * ps.len, 1);
    if (!ctx.avg_vel)
        logging_log(LOG_FATAL, "%s:%d Could not allocate memory. Buy more RAM I guess lol", __FILE__, __LINE__);

    ctx.inst_vel = calloc(sizeof(*ctx.inst_vel) * ps.len, 1);
    if (!ctx.inst_vel)
        logging_log(LOG_FATAL, "%s:%d Could not allocate memory. Buy more RAM I guess lol", __FILE__, __LINE__);

    if (ctx.params.interval_for_information == 0)
        ctx.params.interval_for_information = ctx.params.total_time / ctx.params.dt + 1;

    ctx.expected_steps = ctx.params.total_time / ctx.params.dt + 1;

    integrate_context_print_info(ctx);
    return ctx;
}

void integrate_context_deinit(IntegrateContext *ctx) {
    free(ctx->ps0.items);
    free(ctx->avg_vel);
    free(ctx->inst_vel);
    fclose(ctx->information_file);
    memset(ctx, 0, sizeof(*ctx));
}

void integrate_context_step(IntegrateContext *ctx) {
    for (u64 i = 0; i < ctx->ps0.len; ++i) {
        v2d force = force_at_particle_rk4(ctx->time, ctx->params.dt, i, ctx->bp.ps.items[i], ctx->bp, ctx->potential, ctx->defects, ctx->params.drive_function, ctx->params.drive_data, ctx->params.temperature_function, ctx->params.temperature_data);
        ctx->ps0.items[i].pos = v2d_add(ctx->bp.ps.items[i].pos, force);
        ctx->ps0.items[i].pos = boundary_condition(ctx->ps0.items[i].pos, ctx->defects.limit_x, ctx->defects.limit_y);
        ctx->avg_vel[i] = v2d_add(ctx->avg_vel[i], v2d_fac(force, 1.0 / (ctx->params.dt * ctx->ps0.len)));
        ctx->inst_vel[i] = v2d_fac(force, 1.0 / ctx->params.dt);
    }
    memcpy(ctx->bp.ps.items, ctx->ps0.items, sizeof(*ctx->bp.ps.items) * ctx->ps0.len);

    if (ctx->step % ctx->params.interval_for_information == 0) {
        if (fprintf(ctx->information_file, "%.15e,", ctx->time) < 0)
            logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);

        for (u64 i = 0; i < ctx->ps0.len; ++i)
            if (fprintf(ctx->information_file, "%.15e,%.15e,", ctx->ps0.items[i].pos.x, ctx->ps0.items[i].pos.y) < 0)
                logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);

        for (u64 i = 0; i < ctx->ps0.len - 1; ++i)
            if (fprintf(ctx->information_file, "%.15e,%.15e,", ctx->inst_vel[i].x, ctx->inst_vel[i].y) < 0)
                logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);
        {
            u64 i = ctx->ps0.len - 1;
            if (fprintf(ctx->information_file, "%.15e,%.15e\n", ctx->inst_vel[i].x, ctx->inst_vel[i].y) < 0)
                logging_log(LOG_FATAL, "%s:%d Could not write to file \"%s\"", __FILE__, __LINE__);
        }
    }

    if (ctx->step % (ctx->expected_steps / 10) == 0)
        logging_log(LOG_INFO, "Integrate %.1f%% ", ctx->time / ctx->params.total_time * 100.0);

    boxed_particles_update(&ctx->bp);
    ctx->time += ctx->params.dt;
    ctx->step += 1;
}

void integrate_context_print_info(IntegrateContext ctx) {
    logging_log(LOG_INFO, "-----------------------");
    logging_log(LOG_INFO, "Integrate Context Info:");
    logging_log(LOG_INFO, "dt: %.15e", ctx.params.dt);
    logging_log(LOG_INFO, "Total time: %.15e", ctx.params.total_time);
    logging_log(LOG_INFO, "Information cut: %"PRIu64, ctx.params.interval_for_information);
    logging_log(LOG_INFO, "Information path: %s", ctx.params.path_for_information);
    logging_log(LOG_INFO, "Particle count: %"PRIu64, ctx.ps0.len);
    logging_log(LOG_INFO, "Boxed particles limits: x(%.15e, %.15e) y(%.15e, %.15e)", ctx.bp.limit_x.p[0], ctx.bp.limit_x.p[1], ctx.bp.limit_y.p[0], ctx.bp.limit_y.p[1]);
    logging_log(LOG_INFO, "Boxed particles boxes (rows x cols): %"PRIu64" x %"PRIu64, ctx.bp.rows, ctx.bp.cols);
    logging_log(LOG_INFO, "Boxed particles dx x dy: %.15e x %.15e", ctx.bp.dx, ctx.bp.dy);
    logging_log(LOG_INFO, "Potential table size: %"PRIu64, ctx.potential.len);
    logging_log(LOG_INFO, "Potential table interval: (%.15e, %.15e)", ctx.potential.value_min, ctx.potential.value_max);
    logging_log(LOG_INFO, "Potential table delta: %.15e", ctx.potential.delta_value);
    logging_log(LOG_INFO, "Defect map rows x cols: %"PRIu64" x %"PRIu64, ctx.defects.rows, ctx.defects.cols);
    logging_log(LOG_INFO, "Defect map interval: x(%.15e, %.15e) y(%.15e, %.15e)", ctx.defects.limit_x.p[0], ctx.defects.limit_x.p[1], ctx.defects.limit_y.p[0], ctx.defects.limit_y.p[1]);
    logging_log(LOG_INFO, "Defect map dx x dy: %.15e x %.15e", ctx.defects.dx, ctx.defects.dy);
    logging_log(LOG_INFO, "Integrate expected steps: %"PRIu64, ctx.expected_steps);
    logging_log(LOG_INFO, "-----------------------");
}

v2d integrate(Particles ps, Table particle_potential, DefectMap defects, IntegrateParams params) {
    IntegrateContext ctx = integrate_context_init(ps, particle_potential, defects, params);
    while (ctx.time <= params.total_time)
        integrate_context_step(&ctx);

    v2d avg_vel = v2d_s(0);
    for (u64 i = 0; i < ctx.ps0.len; ++i)
        avg_vel = v2d_add(avg_vel, ctx.avg_vel[i]);
    avg_vel = v2d_fac(avg_vel, 1.0 / ctx.step);
    integrate_context_deinit(&ctx);
    return avg_vel;
}
