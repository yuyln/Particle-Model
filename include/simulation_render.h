#ifndef __SIMULATION_RENDER_H
#define __SIMULATION_RENDER_H
#include "integrate.h"
#include "colors.h"

extern u64 steps_per_frame;

void simulation_render_integrate(Particles ps, Table particle_potential, DefectMap defects, IntegrateParams iparams, u64 width, u64 height);
#endif
