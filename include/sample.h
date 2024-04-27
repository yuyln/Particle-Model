#ifndef __SAMPLE_H
#define __SAMPLE_H
#include "primitive_types.h"
#include "table.h"

typedef struct {
    Table particle_potential;
    Table particle_force;
} SampleParam;

#endif
