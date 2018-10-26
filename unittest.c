#include "vendor/scut/scut.h"

void test_urljoin(struct TestResult *tr);
void test_mpd_manifest_parse_time(struct TestResult *tr);
void test_mpd_manifest_parse_numbers(struct TestResult *tr);
void test_hls_parse_master_playlist(struct TestResult *tr);
void test_hls_parse_whitespace_in_codec(struct TestResult *tr);
void test_hls_parse_media_playlist(struct TestResult *tr);
void test_get_streams_of_hls_master_playlist(struct TestResult *tr);
void test_get_streams_of_hls_media_playlist(struct TestResult *tr);
void test_mpd_streamlist_parse_manifest(struct TestResult *tr);

int main() {
    void (*test_functions[])(struct TestResult *) = {
        &test_urljoin,
        &test_mpd_manifest_parse_time,
        &test_mpd_manifest_parse_numbers,
        &test_hls_parse_master_playlist,
        &test_hls_parse_whitespace_in_codec,
        &test_hls_parse_media_playlist,
        &test_get_streams_of_hls_master_playlist,
        &test_get_streams_of_hls_media_playlist,
        &test_mpd_streamlist_parse_manifest};

    struct TestResult *tr =
        t_run(sizeof test_functions / sizeof test_functions[0], test_functions);
    tr_print(tr);

    _Bool success = tr_success(tr);
    tr_free(tr);
    return success ? 0 : 1;
}
