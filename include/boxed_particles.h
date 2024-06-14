#ifndef __BOXED_PARTICLES_H
#define __BOXED_PARTICLES_H
#include "particles.h"

typedef struct {
    Particles ps;
    u64 rows, cols;
    u64 *indices;
    u64 *n_particle;
    f64 dx, dy;
    v2d limit_x, limit_y;
} BoxedParticles;

BoxedParticles boxed_particles_init(u64 rows, u64 cols, v2d limit_x, v2d limit_y, Particles ps);
u64 boxed_particles_get_idx(BoxedParticles bp, f64 x);
u64 boxed_particles_get_idy(BoxedParticles bp, f64 y);
u64 *boxed_particles_get_indices(BoxedParticles bp, u64 row, u64 col);
void boxed_particles_update(BoxedParticles *bp);
void boxed_particles_deinit(BoxedParticles *bp);

#endif
