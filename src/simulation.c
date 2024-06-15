#include "simulation.h"
#include "macros.h"
#include "logging.h"

f64 system_energy(BoxedParticles bp, Table potential, DefectMap defect_map) {
    f64 energy = 0;
    for (u64 ip = 0; ip < bp.ps.len; ++ip) {
        v2d xy = bp.ps.items[ip].pos;
        s64 idx = boxed_particles_get_idx(bp, xy.x);
        s64 idy = boxed_particles_get_idx(bp, xy.y);
        for (s32 dy = -1; dy <= 1; ++dy) {
            for (s32 dx = -1; dx <= 1; ++dx) {
                u64 idy_interaction = (((idy + dy) % (s64)bp.rows) + (s64)bp.rows) % bp.rows;
                u64 idx_interaction = (((idx + dx) % (s64)bp.cols) + (s64)bp.cols) % bp.cols;
                u64 i_interaction = idy_interaction * bp.cols + idx_interaction;
                u64 *indices_current = boxed_particles_get_indices(bp, idy_interaction, idx_interaction);

                v2d xy_local = v2d_c((xy.x - idx * bp.dx) + ((s64)idx_interaction - dx) * bp.dx,
                                     (xy.y - idy * bp.dy) + ((s64)idy_interaction - dy) * bp.dy);
                for (u64 n = 0; n < bp.n_particle[i_interaction]; ++n)
                    if (!CLOSE_ENOUGH(xy.x, bp.ps.items[indices_current[n]].pos.x, EPS) &&
                        !CLOSE_ENOUGH(xy.y, bp.ps.items[indices_current[n]].pos.y, EPS))
                        energy += 0.5 * particle_energy_point(bp.ps.items[indices_current[n]], xy_local, potential);
            }
        }
        energy += defect_map_potential_xy(xy.x, xy.y, defect_map);
    }
    return energy;
}

v2d force_at_xy(v2d xy, BoxedParticles bp, Table potential, DefectMap defect_map) {
    v2d force = v2d_s(0);
    s64 idx = boxed_particles_get_idx(bp, xy.x);
    s64 idy = boxed_particles_get_idy(bp, xy.y);
    for (s32 dy = -1; dy <= 1; ++dy) {
        for (s32 dx = -1; dx <= 1; ++dx) {
            u64 idy_interaction = (((idy + dy) % (s64)bp.rows) + (s64)bp.rows) % bp.rows;
            u64 idx_interaction = (((idx + dx) % (s64)bp.cols) + (s64)bp.cols) % bp.cols;
            u64 i_interaction = idy_interaction * bp.cols + idx_interaction;
            u64 *indices_current = boxed_particles_get_indices(bp, idy_interaction, idx_interaction);

            v2d xy_local = v2d_c((xy.x - idx * bp.dx) + ((s64)idx_interaction - dx) * bp.dx,
                                 (xy.y - idy * bp.dy) + ((s64)idy_interaction - dy) * bp.dy);

            for (u64 n = 0; n < bp.n_particle[i_interaction]; ++n)
                if (!(CLOSE_ENOUGH(xy_local.x, bp.ps.items[indices_current[n]].pos.x, EPS) &&
                      CLOSE_ENOUGH(xy_local.y, bp.ps.items[indices_current[n]].pos.y, EPS)))
                    force = v2d_add(force, particle_force_point(bp.ps.items[indices_current[n]], xy_local, potential));
        }
    }
    force = v2d_add(force, defect_map_force_xy(xy.x, xy.y, defect_map));
    return force;
}

f64 energy_at(v2d xy, BoxedParticles bp, Table potential, DefectMap defect_map) {
    f64 energy = 0;
    s64 idx = boxed_particles_get_idx(bp, xy.x);
    s64 idy = boxed_particles_get_idy(bp, xy.y);
    for (s32 dy = -1; dy <= 1; ++dy) {
        for (s32 dx = -1; dx <= 1; ++dx) {
            u64 idy_interaction = (((idy + dy) % (s64)bp.rows) + (s64)bp.rows) % bp.rows;
            u64 idx_interaction = (((idx + dx) % (s64)bp.cols) + (s64)bp.cols) % bp.cols;
            u64 i_interaction = idy_interaction * bp.cols + idx_interaction;
            u64 *indices_current = boxed_particles_get_indices(bp, idy_interaction, idx_interaction);

            v2d xy_local = v2d_c((xy.x - idx * bp.dx) + ((s64)idx_interaction - dx) * bp.dx,
                                 (xy.y - idy * bp.dy) + ((s64)idy_interaction - dy) * bp.dy);

            for (u64 n = 0; n < bp.n_particle[i_interaction]; ++n)
                if (!(CLOSE_ENOUGH(xy_local.x, bp.ps.items[indices_current[n]].pos.x, EPS) &&
                      CLOSE_ENOUGH(xy_local.y, bp.ps.items[indices_current[n]].pos.y, EPS)))
                    energy += particle_energy_point(bp.ps.items[indices_current[n]], xy_local, potential);
        }
    }
    energy += defect_map_potential_xy(xy.x, xy.y, defect_map);
    return energy;
}
