#ifndef string_h_INCLUDED
#define string_h_INCLUDED

#include <stdbool.h>

static inline bool str_starts_with(const char *str, const char *prefix) {
    while (*prefix != '\0') {
        if (*prefix++ != *str++) {
            return false;
        }
    }
    return true;
}

#endif // string_h_INCLUDED
