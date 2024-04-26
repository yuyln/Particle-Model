#ifndef __String_H
#define __String_H
#include "primitive_types.h"

#define S_FMT "%.*s"
#define S_ARG(s) (s32)((s).len), (s).items
#define STR_NULL ((String){0})

typedef struct {
    char *items;
    u64 len;
    u64 cap;
} String;

void str_cat_str(String *s, String s2);
void str_cat_cstr(String *s, const char *s2);
void str_cat_fmt(String *s, const char *fmt, ...);
void str_free(String *s);
String str_from_cstr(const char *s);
String str_from_fmt(const char *fmt, ...);
const char *str_as_cstr(String *s);
#endif
