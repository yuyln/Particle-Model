#include "particles.h"
#include "logging.h"
#include "macros.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

bool particles_dump_file(FILE *f, Particles ps) {
    u64 s = fwrite(ps.items, sizeof(*ps.items) * ps.len, 1, f);
    if (s != 1) {
        logging_log(LOG_ERROR, "Could not dump Particles information in file");
        return false;
    }
    return true;
}

bool particles_dump_path(String path, Particles ps) {
    bool ret = true;
    FILE *f = fopen(str_as_cstr(&path), "w");
    if (!f) {
        logging_log(LOG_ERROR, "Could not open file "S_FMT": %s", S_ARG(path), strerror(errno));
        return false;
    }

    u64 intended = sizeof(*ps.items) * ps.len;
    u64 s = fwrite(ps.items, 1, intended, f);
    if (s != intended) {
        logging_log(LOG_ERROR, "Could not dump Particles information (expected to dump %"PRIu64" bytes, but dumped %"PRIu64" bytes) in file "S_FMT": %s", intended, s, S_ARG(path), strerror(errno));
        ret = false;
    }

    if (fclose(f) != 0) {
        logging_log(LOG_ERROR, "Could not close file "S_FMT": %s", S_ARG(path), strerror(errno));
        ret = false;
    }
    return ret;
}

bool particles_append_path(String path, Particles ps) {
    bool ret = true;
    FILE *f = fopen(str_as_cstr(&path), "a");
    if (!f) {
        logging_log(LOG_ERROR, "Could not open file "S_FMT": %s", S_ARG(path), strerror(errno));
        return false;
    }

    u64 intended = sizeof(*ps.items) * ps.len;
    u64 s = fwrite(ps.items, 1, intended, f);
    if (s != intended) {
        logging_log(LOG_ERROR, "Could not dump Particles information (expected to dump %"PRIu64" bytes, but dumped %"PRIu64" bytes) in file "S_FMT": %s", intended, s, S_ARG(path), strerror(errno));
        ret = false;
    }

    if (fclose(f) != 0) {
        logging_log(LOG_ERROR, "Could not close file "S_FMT": %s", S_ARG(path), strerror(errno));
        ret = false;
    }
    return ret;
}

void particles_append_particle(Particles *ps, Particle p) {
    da_append(ps, p);
}

f64 particle_energy_point(Particle p, v2d point, const Table potential) {
    v2d distance_v2d = v2d_sub(point, p.pos);
    f64 distance_squared = v2d_dot(distance_v2d, distance_v2d);
    return p.u0 * table_get_value(potential, sqrt(distance_squared));
}

v2d particle_force_point(Particle p, v2d point, const Table potential) {
    v2d distance_v2d = v2d_sub(point, p.pos);
    f64 distance = sqrt(v2d_dot(distance_v2d, distance_v2d));

    distance_v2d = v2d_fac(distance_v2d, 1.0 / distance);

    return v2d_fac(distance_v2d, -p.u0 * table_get_derivative(potential, distance));
}
