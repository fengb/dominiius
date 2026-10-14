#include <stddef.h>

typedef struct strutil_t {
    const char *str;
    size_t length;
} strutil_t;

const char *strutil_find(const char *haystack, size_t size, const char *needle);