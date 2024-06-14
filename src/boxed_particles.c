#include "boxed_particles.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>

BoxedParticles boxed_particles_init(u64 rows, u64 cols, v2d limit_x, v2d limit_y, Particles ps) {
    f64 dx = (limit_x.p[1] - limit_x.p[0]) / cols;
    f64 dy = (limit_y.p[1] - limit_y.p[0]) / rows;
    BoxedParticles ret = {.rows = rows, .cols = cols, .limit_x = limit_x, .limit_y = limit_y, .dx = dx, .dy = dy, .ps = ps};

    ret.indices = calloc(sizeof(*ret.indices) * rows * cols * ps.len, 1);
    if (!ret.indices)
        logging_log(LOG_FATAL, "Could not allocate indices for boxed particles");

    ret.n_particle = calloc(sizeof(*ret.n_particle) * rows * cols, 1);
    if (!ret.n_particle)
        logging_log(LOG_FATAL, "Could not allocate n_particle for boxed particles");

    return ret;
}

u64 boxed_particles_get_idx(BoxedParticles bp, f64 x) {
    if (x >= bp.limit_x.p[0] && x < bp.limit_x.p[1]) 
        return (x - bp.limit_x.p[0]) / (bp.limit_x.p[1] - bp.limit_x.p[0]) * bp.cols;
    return 0;
}

u64 boxed_particles_get_idy(BoxedParticles bp, f64 y) {
    if (y >= bp.limit_y.p[0] && y < bp.limit_y.p[1]) 
        return (y - bp.limit_y.p[0]) / (bp.limit_y.p[1] - bp.limit_y.p[0]) * bp.rows;
    return 0;
}

void boxed_particles_update(BoxedParticles *bp) {
    for (u64 i = 0; i < bp->rows * bp->cols; ++i)
        bp->n_particle[i] = 0;

    for (u64 i = 0; i < bp->ps.len; ++i) {
        Particle *p = &bp->ps.items[i];
        u64 idx = boxed_particles_get_idx(*bp, p->pos.x);
        u64 idy = boxed_particles_get_idy(*bp, p->pos.y);
        u64 ib = idy * bp->cols + idx;
        bp->indices[bp->n_particle[ib] + bp->ps.len * idy + bp->ps.len * bp->rows * idx] = i;
        bp->n_particle[ib] += 1;
    }
}

u64 *boxed_particles_get_indices(BoxedParticles bp, u64 row, u64 col) {
    if (row < bp.rows && col < bp.cols)
        return &bp.indices[bp.ps.len * row + bp.ps.len * bp.rows * col];
    return &bp.indices[0];
}

void boxed_particles_deinit(BoxedParticles *bp) {
    free(bp->indices);
    free(bp->n_particle);
    memset(bp, 0, sizeof(*bp));
}
