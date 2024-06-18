#include "simulation.h"
#include "macros.h"
#include "logging.h"
#include "utils.h"

#include <math.h>
#include <string.h>

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

v2d force_at_xy(u64 ignore_index, v2d xy, BoxedParticles bp, Table potential, DefectMap defect_map) {
    v2d force = v2d_s(0);
    s64 idx = boxed_particles_get_idx(bp, xy.x);
    s64 idy = boxed_particles_get_idy(bp, xy.y);
    bp.ps.items[ignore_index].force_others = v2d_s(0);
    for (s32 dy = -1; dy <= 1; ++dy) {
        for (s32 dx = -1; dx <= 1; ++dx) {
            u64 idy_interaction = (((idy + dy) % (s64)bp.rows) + (s64)bp.rows) % bp.rows;
            u64 idx_interaction = (((idx + dx) % (s64)bp.cols) + (s64)bp.cols) % bp.cols;
            u64 i_interaction = idy_interaction * bp.cols + idx_interaction;
            u64 *indices_current = boxed_particles_get_indices(bp, idy_interaction, idx_interaction);

            v2d xy_local = v2d_c((xy.x - idx * bp.dx) + ((s64)idx_interaction - dx) * bp.dx,
                                 (xy.y - idy * bp.dy) + ((s64)idy_interaction - dy) * bp.dy);

            for (u64 n = 0; n < bp.n_particle[i_interaction]; ++n) {
                if (CLOSE_ENOUGH(xy_local.x, bp.ps.items[indices_current[n]].pos.x, EPS) &&
                    CLOSE_ENOUGH(xy_local.y, bp.ps.items[indices_current[n]].pos.y, EPS))
                    continue;
                if (indices_current[n] == ignore_index)
                    continue;
                if (bp.particles_interacted[ignore_index * bp.ps.len + indices_current[n]])
                    continue;
                v2d force_between = particle_force_point(bp.ps.items[indices_current[n]], xy_local, potential);
                bp.ps.items[ignore_index].force_others = v2d_add(bp.ps.items[ignore_index].force_others, force_between);
                bp.ps.items[indices_current[n]].force_others = v2d_sub(bp.ps.items[indices_current[n]].force_others, force_between);
                bp.particles_interacted[ignore_index * bp.ps.len + indices_current[n]] = true;
                bp.particles_interacted[indices_current[n] * bp.ps.len + ignore_index] = true;
            }
        }
    }
    force = v2d_add(force, bp.ps.items[ignore_index].force_others);
    force = v2d_add(force, defect_map_force_xy(xy.x, xy.y, defect_map));
    return force;
}

v2d force_at_particle_rk4(f64 t, f64 dt, u64 index, Particle p, BoxedParticles bp, Table potential, DefectMap defect_map, v2d(*drive_fun)(f64, v2d, void *), void *drive_data, f64(*temp_func)(f64, v2d, void *), void *temp_data) {
    v2d rk1, rk2, rk3, rk4;

    v2d temp_vector = v2d_s(0);
    f64 den = p.magnus * p.magnus + p.damping * p.damping;
    f64 temp = 0;

    if (temp_func)
        temp = temp_func(t, p.pos, temp_data);

    if (temp > 0) {
        u64 seed = *(u64*)(&p.pos.x);
        temp_vector.x = sqrt(temp / dt) * normal_distribution(&seed);
        temp_vector.y = sqrt(temp / dt) * normal_distribution(&seed);
    }

    v2d force = temp_vector;
    force = v2d_add(force, force_at_xy(index, p.pos, bp, potential, defect_map));

    if (drive_fun)
        force = v2d_add(force, drive_fun(t, p.pos, drive_data));

    rk1.x = (p.damping * force.x + p.magnus * force.y) / den;
    rk1.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);

    force = temp_vector;
    force = v2d_add(force, force_at_xy(index, v2d_add(p.pos, v2d_fac(rk1, dt * 0.5)), bp, potential, defect_map));

    if (drive_fun)
        force = v2d_add(force, drive_fun(t, v2d_add(p.pos, v2d_fac(rk1, dt * 0.5)), drive_data));

    rk2.x = (p.damping * force.x + p.magnus * force.y) / den;
    rk2.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);

    force = temp_vector;
    force = v2d_add(force, force_at_xy(index, v2d_add(p.pos, v2d_fac(rk2, dt * 0.5)), bp, potential, defect_map));

    if (drive_fun)
        force = v2d_add(force, drive_fun(t, v2d_add(p.pos, v2d_fac(rk2, dt * 0.5)), drive_data));

    rk3.x = (p.damping * force.x + p.magnus * force.y) / den;
    rk3.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);

    force = temp_vector;
    force = v2d_add(force, force_at_xy(index, v2d_add(p.pos, v2d_fac(rk3, dt)), bp, potential, defect_map));

    if (drive_fun)
        force = v2d_add(force, drive_fun(t, v2d_add(p.pos, v2d_fac(rk3, dt)), drive_data));

    rk4.x = (p.damping * force.x + p.magnus * force.y) / den;
    rk4.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);

    return v2d_fac(v2d_add(v2d_add(rk1, v2d_fac(rk2, 2.0)), v2d_add(v2d_fac(rk3, 2.0), rk4)), dt / 6.0);
}

v2d force_at_particle_rk2(f64 t, f64 dt, u64 index, Particle p, BoxedParticles bp, Table potential, DefectMap defect_map, v2d(*drive_fun)(f64, v2d, void *), void *drive_data, f64(*temp_func)(f64, v2d, void *), void *temp_data) {
    v2d rk1, rk2;

    v2d temp_vector = v2d_s(0);
    f64 den = p.magnus * p.magnus + p.damping * p.damping;
    f64 temp = 0;

    if (temp_func)
        temp = temp_func(t, p.pos, temp_data);

    if (temp > 0) {
        u64 seed = *(u64*)(&p.pos.x);
        temp_vector.x = sqrt(temp / dt) * normal_distribution(&seed);
        temp_vector.y = sqrt(temp / dt) * normal_distribution(&seed);
    }

    v2d force = temp_vector;
    force = v2d_add(force, force_at_xy(index, p.pos, bp, potential, defect_map));

    if (drive_fun)
        force = v2d_add(force, drive_fun(t, p.pos, drive_data));

    rk1.x = (p.damping * force.x + p.magnus * force.y) / den;
    rk1.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);

    force = temp_vector;
    force = v2d_add(force, force_at_xy(index, v2d_add(p.pos, v2d_fac(rk1, dt)), bp, potential, defect_map));

    if (drive_fun)
        force = v2d_add(force, drive_fun(t, v2d_add(p.pos, v2d_fac(rk1, dt)), drive_data));

    rk2.x = (p.damping * force.x + p.magnus * force.y) / den;
    rk2.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);


    return v2d_fac(v2d_add(rk1, rk2), dt / 2.0);
}

v2d force_at_particle_euler(f64 t, f64 dt, u64 index, Particle p, BoxedParticles bp, Table potential, DefectMap defect_map, v2d(*drive_fun)(f64, v2d, void *), void *drive_data, f64(*temp_func)(f64, v2d, void *), void *temp_data) {
    v2d ret;
    v2d temp_vector = v2d_s(0);
    f64 den = p.magnus * p.magnus + p.damping * p.damping;
    f64 temp = 0;

    if (temp_func)
        temp = temp_func(t, p.pos, temp_data);

    if (temp > 0) {
        u64 seed = *(u64*)(&p.pos.x);
        temp_vector.x = sqrt(temp / dt) * normal_distribution(&seed);
        temp_vector.y = sqrt(temp / dt) * normal_distribution(&seed);
    }

    v2d force = temp_vector;
    force = v2d_add(force, force_at_xy(index, p.pos, bp, potential, defect_map));
    if (drive_fun)
        force = v2d_add(force, drive_fun(t, p.pos, drive_data));
    ret.x = (p.damping * force.x + p.magnus * force.y) / den;
    ret.y = (p.damping * force.y - p.magnus * force.x) / den;
    memset(&bp.particles_interacted[index * bp.ps.len], 0, sizeof(*bp.particles_interacted) * bp.ps.len);

    return v2d_fac(ret, dt);
}
