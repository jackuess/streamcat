#ifndef vector_h_INCLUDED
#define vector_h_INCLUDED

#include <stddef.h>
#include <stdlib.h>

#define VECTOR_INIT_CAPACITY 4

struct Vector {
    size_t _capacity;
    size_t len;
    void *items[];
};

struct Vector *vector_init()
{
    struct Vector *vec = malloc(offsetof(struct Vector, items) + VECTOR_INIT_CAPACITY * sizeof vec->items[0]);
    if (vec != NULL) {
        vec->_capacity = VECTOR_INIT_CAPACITY;
        vec->len = 0;
    }
    return vec;
}

void vector_free(struct Vector *vec)
{
    for (size_t i = 0; i < vec->len; i++) {
        free(vec->items[i]);
    }
    free(vec);
}

struct Vector *vector_append(struct Vector *vec, void *item)
{
    if (vec->_capacity == vec->len) {
    	vec = realloc(vec, offsetof(struct Vector, items) + vec->_capacity * 2 * sizeof vec->items[0]);
    }
    if (vec != NULL) {
        vec->items[vec->len++] = item;
    }
    return vec;
}

#endif // vector_h_INCLUDED
