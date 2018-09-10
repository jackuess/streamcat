#include <stdlib.h>

#include "http.h"
#include "minunit.h"

void test_urljoin(struct TestResult *tr) {
    char *joined = urljoin("http://foo.bar/", "baz");
    ASSERT_STR_EQ(tr, joined, "http://foo.bar/baz");
    free(joined);

    joined = urljoin("http://foo.bar/foo/bar/foobar", "baz");
    ASSERT_STR_EQ(tr, joined, "http://foo.bar/foo/bar/baz");
    free(joined);

    joined = urljoin("http://foo.bar/foo/bar/foobar", "/baz");
    ASSERT_STR_EQ(tr, joined, "http://foo.bar/foo/bar/baz");
    free(joined);

    joined = urljoin("http://foo.bar/foo/bar/foobar", "/baz");
    ASSERT_STR_EQ(tr, joined, "http://foo.bar/foo/bar/baz");
    free(joined);
}
