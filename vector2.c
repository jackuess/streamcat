#include <assert.h>
#include <stdlib.h>

#include "vector2.h"

struct Header {
    size_t item_size;
    size_t cap;
    size_t len;
};

#define VECHEAD(data) ((struct Header *)((char*)data - sizeof (struct Header)))
#define VECDATA(head) (head + 1)

void *vecnew(size_t nmemb, size_t item_size) {
    struct Header *head = malloc(sizeof (*head) + nmemb * item_size);  // TODO(Jacques): Handle failure
    head->item_size = item_size;
    head->cap = nmemb;
    head->len = 0;
    return VECDATA(head);
}

void *vecgrow(struct Header *head, size_t required_len) {
    size_t newcap = required_len * 2;
    head = realloc(head, sizeof (*head) + newcap * head->item_size);  // TODO(Jacques): Handle failure
    head->cap = newcap;
    return head;
}

void *vecappend(void *dest, const void* src) {
    struct Header *head = VECHEAD(dest);

    if (head->len + 1 > head->cap) {
        head = vecgrow(head, head->len + 1);
        dest = VECDATA(head);
    }

    if (src != NULL) {
        char *bytes_dest = dest;
        const char *bytes_src = src;
        bytes_dest[head->len * head->item_size] = bytes_src[0];
    }
    head->len++;

    return dest;
}

void *vecextend(void *dest, const void* src, size_t n) {
    struct Header *head = VECHEAD(dest);

    if (head->len + n > head->cap) {
        head = vecgrow(head, head->len + n);
        dest = VECDATA(head);
    }

    char *bytes_dest = dest;
    const char *bytes_src = src;
    for (size_t i = 0; i < n; i++) {
        bytes_dest[head->len++ * head->item_size] = bytes_src[i * head->item_size];
    }

    return dest;
}

size_t veclen(const void *data) {
    struct Header *head = VECHEAD(data);
    return head->len;
}

void* vecsetlen(void *data, size_t newlen) {
    struct Header *head = VECHEAD(data);
    if (newlen > head->cap) {
        head = vecgrow(head, newlen);
    }
    head->len = newlen;
    return VECDATA(head);
}

void vecfree(void *data) {
    struct Header *head = VECHEAD(data);
    free(head);
}
