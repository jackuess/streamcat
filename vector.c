#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

struct Header {
    size_t cap;
    size_t len;
    size_t item_size;
    unsigned int padding: (sizeof (size_t) * 3) % alignof (max_align_t);
};

static inline struct Header *vechead(const void *data) {
    return (struct Header *)((unsigned char*)data - sizeof (struct Header));
}

static void *vecsetcap(void *data, size_t item_size, size_t cap) {
    const size_t data_offset = sizeof (struct Header);
    unsigned char *buf = NULL;

    if (data == NULL) {
        buf = malloc(data_offset + cap * item_size);
    } else {
        buf = realloc(vechead(data), data_offset + cap * item_size);
    }
    if (buf == NULL) {
        return NULL;
    }

    return buf + data_offset;
}

void *vecnew(size_t nmemb, size_t item_size) {
    void *data = vecsetcap(NULL, item_size, nmemb);
    if (data == NULL) {
        return NULL;
    }

    struct Header *head = vechead(data);;
    head->cap = nmemb;
    head->len = 0;
    head->item_size = item_size;

    return data;
}

void *vecextend(void **data, size_t n) {
    struct Header *head = vechead(*data);
    if (head->len + n > head->cap) {
        size_t new_cap = 2 * (head->len + n);
        *data = vecsetcap(*data, head->item_size, new_cap);
        if (*data == NULL) {
            return NULL;
        }
        head = vechead(*data);
        head->cap = new_cap;
    }

    unsigned char *data_bytes = *data;
    void *extension = &data_bytes[head->len * head->item_size];

    head->len += n;

    return extension;
}

void *vecappend(void **data) {
    return vecextend(data, 1);
}

size_t veclen(const void *data) {
    struct Header *head = vechead(data);
    return head->len;
}

void vecfree(void *data) {
    free(vechead(data));
}
