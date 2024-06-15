#ifndef __SIMULATION_H
#define __SIMULATION_H
#include "defect_map.h"
#include "table.h"
#include "boxed_particles.h"

f64 system_energy(BoxedParticles ps, Table potential, DefectMap defect_map);
f64 energy_at(v2d xy, BoxedParticles bp, Table potential, DefectMap defect_map);
v2d force_at_xy(u64 ignore_index, v2d xy, BoxedParticles bp, Table potential, DefectMap defect_map);
v2d force_at_particle_rk4(f64 t, f64 dt, u64 index, Particle p, BoxedParticles bp, Table potential, DefectMap defect_map, v2d(*drive_fun)(f64, v2d, void *), void *drive_data, f64(*temp_func)(f64, v2d, void *), void *temp_data);
v2d force_at_particle_rk2(f64 t, f64 dt, u64 index, Particle p, BoxedParticles bp, Table potential, DefectMap defect_map, v2d(*drive_fun)(f64, v2d, void *), void *drive_data, f64(*temp_func)(f64, v2d, void *), void *temp_data);
v2d force_at_particle_euler(f64 t, f64 dt, u64 index, Particle p, BoxedParticles bp, Table potential, DefectMap defect_map, v2d(*drive_fun)(f64, v2d, void *), void *drive_data, f64(*temp_func)(f64, v2d, void *), void *temp_data);

#endif
