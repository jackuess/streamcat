/*
 * Copyright 2019 Jacques de Laval

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
