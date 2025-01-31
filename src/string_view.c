#include "string_view.h"
#include "logging.h"
#include "macros.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void sb_cat_sb(StringBuilder *s, StringBuilder s2) {
    for (uint64_t i = 0; i < s2.len; ++i)
        da_append(s, s2.items[i]);
}

void sb_cat_cstr(StringBuilder *s, const char *s2) {
    uint64_t s2_len = strlen(s2);
    for (uint64_t i = 0; i < s2_len; ++i)
        da_append(s, s2[i]);
}

void sb_cat_fmt(StringBuilder *s, const char *fmt, ...) {
    char *tmp = NULL;

    va_list arg_list;
    va_start(arg_list, fmt);
    if (!fmt) {
        logging_log(LOG_WARNING, "Format string_builder provided is NULL");
        goto err;
    }

    uint64_t s2_len = vsnprintf(NULL, 0, fmt, arg_list) + 1;
    va_end(arg_list);

    tmp = malloc(s2_len);
    assert(tmp != NULL);

    va_start(arg_list, fmt);
    vsnprintf(tmp, s2_len, fmt, arg_list);
    va_end(arg_list);
    for (uint64_t i = 0; i < s2_len; ++i)
    da_append(s, tmp[i]);
err: 
    free(tmp);
}

void sb_free(StringBuilder *s) {
    free(s->items);
    memset(s, 0, sizeof(*s));
}

const char *sb_as_cstr(StringBuilder *s) {
    if (s->items[s->len - 1] != '\0')
        da_append(s, '\0');
    return s->items;
}
