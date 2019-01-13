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

#include "../vendor/scut/scut.h"

void test_urljoin(struct TestResult *tr);
void test_mpd_manifest_parse_time(struct TestResult *tr);
void test_mpd_manifest_parse_numbers(struct TestResult *tr);
void test_hls_parse_master_playlist(struct TestResult *tr);
void test_hls_parse_whitespace_in_codec(struct TestResult *tr);
void test_hls_parse_media_playlist(struct TestResult *tr);
void test_get_streams_of_hls_master_playlist(struct TestResult *tr);
void test_get_streams_of_hls_media_playlist(struct TestResult *tr);
void test_get_segments_of_hls_stream(struct TestResult *tr);
void test_mpd_streamlist_parse_manifest(struct TestResult *tr);
void test_get_segments_of_mpd_stream(struct TestResult *tr);

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
        &test_get_segments_of_hls_stream,
        &test_mpd_streamlist_parse_manifest,
        &test_get_segments_of_mpd_stream};

    struct TestResult *tr =
        t_run(sizeof test_functions / sizeof test_functions[0], test_functions);
    tr_print(tr);

    _Bool success = tr_success(tr);
    tr_free(tr);
    return success ? 0 : 1;
}
