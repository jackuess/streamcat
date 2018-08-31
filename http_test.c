#include "http.h"
#include "minunit.h"

char *test_urljoin()
{
    char *joined = urljoin("http://foo.bar/", "baz");
    ASSERT_STR_EQ(joined, "http://foo.bar/baz");
    free(joined);

    joined = urljoin("http://foo.bar/foo/bar/foobar", "baz");
    ASSERT_STR_EQ(joined, "http://foo.bar/foo/bar/baz");
    free(joined);

    joined = urljoin("http://foo.bar/foo/bar/foobar", "/baz");
    ASSERT_STR_EQ(joined, "http://foo.bar/foo/bar/baz");
    free(joined);

    joined = urljoin("http://foo.bar/foo/bar/foobar", "/baz");
    ASSERT_STR_EQ(joined, "http://foo.bar/foo/bar/baz");
    free(joined);

    return NULL;
}
