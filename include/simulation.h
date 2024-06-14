#ifndef __SIMULATION_H
#define __SIMULATION_H
#include "defect_map.h"
#include "table.h"
#include "boxed_particles.h"

f64 system_energy(BoxedParticles ps, Table potential, DefectMap defect_map);
v2d force_at_xy(v2d xy, BoxedParticles ps, Table potential, DefectMap defect_map);

#endif
