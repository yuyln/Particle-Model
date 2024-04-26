#include "string_view.h"
#include "logging.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

u64 vfmt_get_size(const char *fmt, va_list args) {
    return vsnprintf(NULL, 0, fmt, args);
}

void str_cat_str(String *s, String s2) {
    if (!s2.items) {
        logging_log(LOG_WARNING, "String argument passed to concatenate (s2) is NULL. Trying to concatenate %.*s.", (int)s->len, s->items);
        return;
    }

    u64 old_len = s->len;
    s->len += s2.len;
    char *old_ptr = s->items;
    s->items = realloc(s->items, s->len + 1);
    if (!s->items)
        logging_log(LOG_FATAL, "Could not realloc pointer of str \"%.*s\" to cat with \"%.*s\": %s", (int)old_len, old_ptr, (int)s2.len, s2.items, strerror(errno));
    else {
        memmove(&s->items[old_len], s2.items, s2.len);
        s->items[s->len] = '\0';
    }
    s->cap = s->len;
}

void str_cat_cstr(String *s, const char *s2) {
    u64 old_len = s->len;
    u64 s2_len = strlen(s2);
    s->len += s2_len;
    char *old_ptr = s->items;
    s->items = realloc(s->items, s->len + 1);
    if (!s->items)
        logging_log(LOG_FATAL, "Could not realloc pointer of str \"%.*s\" to cat with \"%s\": %s", (int)old_len, old_ptr, s2, strerror(errno));
    else
        memmove(&s->items[old_len], s2, s2_len + 1);
    s->cap = s->len;
}

void str_cat_fmt(String *s, const char *fmt, ...) {
    char *tmp = NULL;

    va_list arg_list;
    va_start(arg_list, fmt);
    if (!fmt) {
        logging_log(LOG_WARNING, "Format String provided is NULL");
        goto err;
    }

    u64 s2_len = vsnprintf(NULL, 0, fmt, arg_list) + 1;
    va_end(arg_list);

    tmp = calloc(s2_len, 1);
    if (!tmp) {
        logging_log(LOG_ERROR, "Could not alloc %"PRIu64" bytes for tmp: %s", s2_len, strerror(errno));
        goto err;
    }

    va_start(arg_list, fmt);
    vsnprintf(tmp, s2_len, fmt, arg_list);
    va_end(arg_list);

    s2_len = strlen(tmp);

    u64 old_len = s->len;
    s->len += s2_len;
    char *old_ptr = s->items;
    s->items = realloc(s->items, s->len + 1);
    if (!s->items) {
        logging_log(LOG_ERROR, "Could not realloc pointer of str \"%.*s\" to cat with \"%s\": %s", (int)old_len, old_ptr, tmp, strerror(errno));
        s->items = old_ptr;
    } else
        memmove(&s->items[old_len], tmp, s2_len + 1);
err: 
    free(tmp);
}

void str_free(String *s) {
    free(s->items);
    memset(s, 0, sizeof(*s));
}

const char *str_as_cstr(String *s) {
    return s->items;
}

String str_from_cstr(const char *s) {
    String ret = STR_NULL;
    str_cat_cstr(&ret, s);
    return ret;
}

String str_from_fmt(const char *fmt, ...) {
    String ret = STR_NULL;
    va_list arg_list;
    va_start(arg_list, fmt);
    if (!fmt) {
        logging_log(LOG_WARNING, "Format String provided is NULL");
        return (String){0};
    }

    u64 tmp_len = vsnprintf(NULL, 0, fmt, arg_list) + 1;
    va_end(arg_list);

    char *tmp = calloc(tmp_len, 1);
    if (!tmp)
        logging_log(LOG_FATAL, "Could not alloc %"PRIu64" bytes for tmp: %s", tmp_len, strerror(errno));

    va_start(arg_list, fmt);
    vsnprintf(tmp, tmp_len, fmt, arg_list);
    va_end(arg_list);

    tmp_len = strlen(tmp);
    ret.len = tmp_len;
    ret.items = calloc(ret.len + 1, 1);
    if (!ret.items)
        logging_log(LOG_FATAL, "Could not alloc pointer for str [%"PRIu64" bytes], returning null String: %s", (int)ret.len, strerror(errno));
    else
        memmove(ret.items, tmp, tmp_len + 1);

    free(tmp);
    ret.cap = ret.len;
    return ret;
}
