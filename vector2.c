#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vector2.h"

struct Header {
    size_t item_size;
    size_t cap;
    size_t len;
};

#define VECHEAD(data) ((struct Header *)((unsigned char*)data - sizeof (struct Header)))
#define VECDATA(head) (head + 1)

void *vecnew(size_t nmemb, size_t item_size) {
    struct Header *head = malloc(sizeof (*head) + nmemb * item_size);  // TODO(Jacques): Handle failure
    head->item_size = item_size;
    head->cap = nmemb;
    head->len = 0;
    return VECDATA(head);
}

static void *vecgrow(struct Header *head, size_t required_len) {
    size_t newcap = required_len * 2;
    head = realloc(head, sizeof (*head) + newcap * head->item_size);  // TODO(Jacques): Handle failure
    head->cap = newcap;
    return head;
}

void *vecappend(void **dest, const void* src) {
    return vecextend(dest, src, 1);
}

void *vecextend(void **dest, const void* src, size_t n) {
    struct Header *head = VECHEAD(*dest);
    if (head->len + n > head->cap) {
        head = vecgrow(head, head->len + n);
        *dest = VECDATA(head);
    }

    unsigned char *bytes_dest = *dest;
    void *extension = &bytes_dest[head->len * head->item_size];

    if (src == NULL) {
        memset(extension, 0, head->item_size * n);
    } else {
        memcpy(extension, src, head->item_size * n);
    }

    head->len += n;
    return extension;
}

size_t veclen(const void *data) {
    struct Header *head = VECHEAD(data);
    return head->len;
}

void vecfree(void *data) {
    struct Header *head = VECHEAD(data);
    free(head);
}
