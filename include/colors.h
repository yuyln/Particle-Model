#ifndef __COLORS_H
#define __COLORS_H
#include "primitive_types.h"

typedef union {
    struct { byte b, g, r, a; };
    struct { byte x, y, z, w; };
    u32 bgra;
} RGBA32;

#endif
