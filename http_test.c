#include "http.h"
#include "minunit.h"

char *test_urljoin()
{
    ASSERT_STR_EQ(urljoin("http://foo.bar/", "baz"), "http://foo.bar/baz");
    ASSERT_STR_EQ(urljoin("http://foo.bar/foo/bar/foobar", "baz"), "http://foo.bar/foo/bar/baz");
    ASSERT_STR_EQ(urljoin("http://foo.bar/foo/bar/foobar", "/baz"), "http://foo.bar/foo/bar/baz");
    ASSERT_STR_EQ(urljoin("http://foo.bar/foo/bar/foobar", "/baz"), "http://foo.bar/foo/bar/baz");

    return NULL;
}
