#include "utils.h"
#include "logging.h"

#include <errno.h>
#include <string.h>

v2d boundary_condition(v2d pos, v2d limit_x, v2d limit_y) {
    if (pos.x < limit_x.p[0] || pos.x > limit_x.p[1])
        pos.x = 0;

    if (pos.y < limit_y.p[0] || pos.y > limit_y.p[1])
        pos.y = 0;

    return pos;
}

bool read_entire_file(String path, String *str) {
    bool ret = true;
    FILE *f = fopen(str_as_cstr(&path), "r");
    if (!f) {
        logging_log(LOG_ERROR, "Could not open file "S_FMT": %s", S_ARG(path), strerror(errno));
        return false;
    }
    char c;
    while ((c = fgetc(f)) != EOF)
        da_append(str, c);

    if (fclose(f) != 0) {
        logging_log(LOG_ERROR, "Could not close file "S_FMT": %s", S_ARG(path), strerror(errno));
        ret = false;
    }

    return ret;
}
