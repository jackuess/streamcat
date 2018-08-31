#include "minunit.h"

char *test_urljoin();
char *test_mpd_manifest_parse_time();
char *test_mpd_manifest_parse_numbers();
char *test_vector();
char *test_vector_extend();
char *test_vector_append_struct();
char* (*test_functions[]) () = {
    &test_urljoin,
    &test_mpd_manifest_parse_time,
    &test_mpd_manifest_parse_numbers,
    &test_vector,
    &test_vector_extend,
    &test_vector_append_struct,
    NULL
};

int main() {
    _Bool success = run_all_tests(test_functions);
    return success ? 0 : 1;
}
