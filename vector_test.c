#include <stdalign.h>
#include <stdint.h>

#include "vector.h"
#include "vendor/scut/scut.h"

void test_vector_append(struct TestResult *tr) {
    char *string = vecnew(1, sizeof (string[0]));
    char *c = NULL;

    c = VECAPPEND(&string);
    c[0] = 'F';
    c = VECAPPEND(&string);
    c[0] = 'O';
    c = VECAPPEND(&string);
    c[0] = 'O';
    ASSERT_EQ(tr, string[2], 'O');
    c[0] = 'o';
    c = VECAPPEND(&string);
    c[0] = 'O';
    c = VECAPPEND(&string);
    c[0] = '\0';
    ASSERT_STR_EQ(tr, string, "FOoO");
    ASSERT_EQ(tr, veclen(string), 5);

    vecfree(string);
}

void test_vector_extend(struct TestResult *tr) {
    char *string = vecnew(1, sizeof (*string));
    char *source = "foobar";
    char *c;

    c = VECAPPEND(&string);
    c[0] = source[1];
    char *extension = VECEXTEND(&string, 7);
    strncpy(extension, source, 7);
    ASSERT_STR_EQ(tr, string, "ofoobar");
    ASSERT_EQ(tr, veclen(string), 8);

    vecfree(string);
}

struct Foo {
    int a;
    int b;
};

void foo_init(struct Foo *foo, int a, int b) {
    foo->a = a;
    foo->b = b;
}

void test_vector_append_struct(struct TestResult *tr) {
    struct Foo *foo = vecnew(1, sizeof (struct Foo));

    foo_init(VECAPPEND(&foo), 5, 6);
    ASSERT_EQ(tr, foo[0].a, 5);
    ASSERT_EQ(tr, foo[0].b, 6);

    struct Foo *appendage = VECAPPEND(&foo);
    appendage->a = 7;
    appendage->b = 8;
    ASSERT_EQ(tr, foo[1].a, 7);
    ASSERT_EQ(tr, foo[1].b, 8);
    ASSERT_EQ(tr, veclen(foo), 2);

    vecfree(foo);
}

void test_vector_memory_alignment(struct TestResult *tr) {
    long double *foo = vecnew(1, sizeof foo[0]);
    ASSERT_EQ(tr, (uintptr_t)(const void *)foo % alignof (foo[0]), 0);
    VECAPPEND(&foo);
    foo[0] = 2.2;
    ASSERT_EQ(tr, foo[0], 2.2);
    ASSERT_EQ(tr, veclen(foo), 1);
    vecfree(foo);
}
