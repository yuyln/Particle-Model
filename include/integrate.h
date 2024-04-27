#ifndef __INTEGRATE_H
#define __INTEGRATE_H

#include "v2d.h"
#include "primitive_types.h"
#include "utils.h"
#include "macros.h"

typedef struct {
    f64 dt;
    f64 duration;

    v2d(*current)(f64, v2d, Box);
    f64(*temperature)(f64, v2d, Box);

    u64 trajectories_cut;
    u64 velocities_cut;
} integrate_params;

#endif
