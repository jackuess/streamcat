#include <stdalign.h>
#include <stdint.h>

#include "minunit.h"
#include "vector.h"

char *test_vector_append() {
    char *string = VECNEW(1, string[0]);
    char *c = NULL;

    c = VECAPPEND(&string);
    c[0] = 'F';
    c = VECAPPEND(&string);
    c[0] = 'O';
    c = VECAPPEND(&string);
    c[0] = 'O';
    ASSERT_EQ(string[2], 'O');
    c[0] = 'o';
    c = VECAPPEND(&string);
    c[0] = 'O';
    c = VECAPPEND(&string);
    c[0] = '\0';
    ASSERT_STR_EQ(string, "FOoO");
    ASSERT_EQ(veclen(string), 5);

    VECFREE(string);

    return NULL;
}

char *test_vector_extend() {
    char *string = VECNEW(1, *string);
    char *source = "foobar";
    char *c;

    c = VECAPPEND(&string);
    c[0] = source[1];
    char *extension = VECEXTEND(&string, 7);
    strncpy(extension, source, 7);
    ASSERT_STR_EQ(string, "ofoobar");
    ASSERT_EQ(veclen(string), 8);

    VECFREE(string);

    return NULL;
}

struct Foo {
    int a;
    int b;
};

void foo_init(struct Foo *foo, int a, int b) {
    foo->a = a;
    foo->b = b;
}

char *test_vector_append_struct() {
    struct Foo *foo = VECNEW(1, struct Foo);

    foo_init(VECAPPEND(&foo), 5, 6);
    ASSERT_EQ(foo[0].a, 5);
    ASSERT_EQ(foo[0].b, 6);

    struct Foo *appendage = VECAPPEND(&foo);
    appendage->a = 7;
    appendage->b = 8;
    ASSERT_EQ(foo[1].a, 7);
    ASSERT_EQ(foo[1].b, 8);
    ASSERT_EQ(veclen(foo), 2);

    VECFREE(foo);

    return NULL;
}

char *test_vector_memory_alignment() {
    long double *foo = VECNEW(1, foo[0]);
    ASSERT_EQ((uintptr_t)(const void *)foo % alignof (foo[0]), 0);
    VECAPPEND(&foo);
    foo[0] = 2.2;
    ASSERT_EQ(foo[0], 2.2);
    ASSERT_EQ(veclen(foo), 1);
    VECFREE(foo);

    return NULL;
}
