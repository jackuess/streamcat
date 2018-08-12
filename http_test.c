#include <string.h>

#include "http.h"
#include "minunit.h"

static char *test_urljoin()
{
    mu_assert(
        "urljoin(\"http://foo.bar/\", \"baz\") != \"http://foo.bar/baz\"",
        strcmp(urljoin("http://foo.bar/", "baz"), "http://foo.bar/baz") == 0
    );
    mu_assert(
        "urljoin(\"http://foo.bar/foo/bar/foobar\", \"baz\") != \"http://foo.bar/foo/bar/baz\"",
        strcmp(urljoin("http://foo.bar/foo/bar/foobar", "baz"), "http://foo.bar/foo/bar/baz") == 0
    );
    mu_assert(
        "urljoin(\"http://foo.bar/foo/bar/foobar\", \"/baz\") != \"http://foo.bar/foo/bar/baz\"",
        strcmp(urljoin("http://foo.bar/foo/bar/foobar", "/baz"), "http://foo.bar/foo/bar/baz") == 0
    );

    return 0;
}

char *all_tests()
{
    mu_run_test(test_urljoin);
    return 0;
}
