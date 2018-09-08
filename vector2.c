#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vector2.h"

struct Header {
    size_t cap;
    size_t len;
};

static inline void vecwritehead(void *data, struct Header *head) {
    memcpy((unsigned char*)data - sizeof (*head), head, sizeof (*head));
}

static inline void vecreadhead(struct Header *head, const void *data) {
    memcpy(head, (unsigned char*)data - sizeof (*head), sizeof (*head));
}

static size_t vecdataoffset(size_t header_size, size_t data_alignment) {
    return header_size + (header_size % data_alignment);
}

static void *vecsetcap(void *data, size_t item_size, size_t alignment, size_t cap) {
    size_t data_offset = vecdataoffset(sizeof (struct Header), alignment);
    unsigned char *buf = NULL;

    if (data == NULL) {
        buf = malloc(data_offset + cap * item_size);
    } else {
        buf = realloc((unsigned char*)data - data_offset, data_offset + cap * item_size);
    }
    if (buf == NULL) {
        return NULL;
    }

    return buf + data_offset;
}

void *vecnew(size_t nmemb, size_t item_size, size_t alignment) {
    struct Header head = {0};

    void *data = vecsetcap(NULL, item_size, alignment, nmemb);
    if (data == NULL) {
        return NULL;
    }

    head.cap = nmemb;
    head.len = 0;
    vecwritehead(data, &head);

    return data;
}

void *vecextend(void **data, size_t item_size, size_t alignment, size_t n) {
    struct Header head;
    vecreadhead(&head, *data);
    if (head.len + n > head.cap) {
        size_t new_cap = 2 * (head.len + n);
        *data = vecsetcap(*data, item_size, alignment, new_cap);
        if (*data == NULL) {
            return NULL;
        }
        head.cap = new_cap;
    }

    unsigned char *data_bytes = *data;
    void *extension = &data_bytes[head.len * item_size];

    head.len += n;
    vecwritehead(*data, &head);

    return extension;
}

void *vecappend(void **data, size_t item_size, size_t alignment) {
    return vecextend(data, item_size, alignment, 1);
}

size_t veclen(const void *data) {
    struct Header head;
    vecreadhead(&head, data);
    return head.len;
}

void vecfree(void *data, size_t alignment) {
    free((unsigned char*)data - vecdataoffset(sizeof (struct Header), alignment));
}
