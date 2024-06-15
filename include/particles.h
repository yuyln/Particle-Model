#ifndef __PARTICLES_H
#define __PARTICLES_H
#include "primitive_types.h"
#include "v2d.h"
#include "string_view.h"
#include "table.h"

#include <stdio.h>

typedef struct {
    v2d pos;
    f64 magnus;
    f64 damping;
    f64 u0;
    f64 radius;
} Particle;

typedef struct {
    Particle *items;
    u64 len;
    u64 cap;
} Particles;

Particles particles_from_file(String path);
bool particles_dump_file(FILE *f, Particles ps);
bool particles_dump_path(String path, Particles ps);
bool particles_append_path(String path, Particles ps);
void particles_append_particle(Particles *ps, Particle p);
f64 particle_energy_point(Particle p, v2d point, const Table potential);
v2d particle_force_point(Particle p, v2d point, const Table force);
#endif
