#include <stdalign.h>
#include <stdint.h>

#include "minunit.h"
#include "vector2.h"

char *test_vector() {
    char *string = vecnew(1, sizeof (char));

    char source[4] = {'F', 'O', 'O', '\0'};
    VECAPPEND(&string, &source[0]);
    VECAPPEND(&string, &source[1]);
    char *c = VECAPPEND(&string, &source[2]);
    ASSERT_EQ(string[2], 'O');
    c[0] = 'o';
    VECAPPEND(&string, &source[2]);
    VECAPPEND(&string, &source[3]);
    ASSERT_STR_EQ(string, "FOoO");
    ASSERT_EQ(veclen(string), 5);

    vecfree(string);

    return NULL;
}

char *test_vector_extend() {
    char *string = vecnew(1, sizeof (char));
    char *source = "foobar";

    VECAPPEND(&string, &source[1]);
    char *extension = VECEXTEND(&string, source, 7);
    ASSERT_STR_EQ(string, "ofoobar");
    ASSERT_STR_EQ(extension, source);
    ASSERT_EQ(veclen(string), 8);

    char *nullextension = VECEXTEND(&string, NULL, 2);
    ASSERT_EQ(nullextension[0], 0);
    ASSERT_EQ(nullextension[1], 0);
    ASSERT_EQ(veclen(string), 10);

    vecfree(string);

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
    struct Foo *foo = vecnew(1, sizeof (struct Foo));

    foo_init(VECAPPEND(&foo, NULL), 5, 6);
    ASSERT_EQ(foo[0].a, 5);
    ASSERT_EQ(foo[0].b, 6);

    struct Foo to_append = {7, 8};
    VECAPPEND(&foo, &to_append);
    ASSERT_EQ(foo[1].a, 7);
    ASSERT_EQ(foo[1].b, 8);
    ASSERT_EQ(veclen(foo), 2);

    vecfree(foo);

    return NULL;
}

char *test_vector_memory_alignment() {
    long double *foo = vecnew(1, sizeof (foo[0]));
    ASSERT_EQ((uintptr_t)(const void *)foo % alignof (foo[0]), 0);
    vecfree(foo);

    return NULL;
}
