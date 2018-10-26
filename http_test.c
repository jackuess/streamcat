#include <stdlib.h>

#include "http.h"
#include "vendor/scut/scut.h"

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

    joined = urljoin("http://foo.bar/", "http://bar.foo/");
    ASSERT_STR_EQ(tr, joined, "http://bar.foo/");
    free(joined);

    joined = urljoin("http://foo.bar/", "https://bar.foo/");
    ASSERT_STR_EQ(tr, joined, "https://bar.foo/");
    free(joined);
}
