#ifndef __STRINGBUILDER_H
#define __STRINGBUILDER_H
#include "primitive_types.h"

#define S_FMT "%.*s"
#define S_ARG(s) (s32)((s).len), (s).items

typedef struct {
    char *items;
    u64 len;
    u64 cap;
} StringBuilder;

void sb_cat_sb(StringBuilder *s, StringBuilder s2);
void sb_cat_cstr(StringBuilder *s, const char *s2);
void sb_cat_fmt(StringBuilder *s, const char *fmt, ...);
void sb_free(StringBuilder *s);
const char *sb_as_cstr(StringBuilder *s);

#endif
