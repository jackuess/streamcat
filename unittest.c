#include "minunit.h"

void test_urljoin(struct TestResult *tr);
void test_mpd_manifest_parse_time(struct TestResult *tr);
void test_mpd_manifest_parse_numbers(struct TestResult *tr);
void test_vector_append(struct TestResult *tr);
void test_vector_extend(struct TestResult *tr);
void test_vector_append_struct(struct TestResult *tr);
void test_vector_memory_alignment(struct TestResult *tr);

int main() {
    void (*test_functions[]) (struct TestResult *) = {
        &test_urljoin,
        &test_mpd_manifest_parse_time,
        &test_mpd_manifest_parse_numbers,
        &test_vector_append,
        &test_vector_extend,
        &test_vector_append_struct,
        &test_vector_memory_alignment
    };

    struct TestResult *tr = t_run(sizeof test_functions / sizeof test_functions[0], test_functions);
    tr_print(tr);

    _Bool success = tr_success(tr);
    tr_free(tr);
    return success ? 0 : 1;
}
