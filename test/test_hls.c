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

#include <string.h>

#include "../vendor/scut/scut.h"

#include "../src/hls.h"

void test_hls_parse_master_playlist(struct TestResult *tr) {
    HLSPlaylist *playlist = hls_playlist_new("http://example.com/master.m3u8");
    struct HLSVariantStream *streams = NULL;
    const char *data =
        "#EXTM3U\n"
        "#A comment\n"
        "#\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=1280000,AVERAGE-BANDWIDTH=1000000\n"
        "http://example.com/low.m3u8\n"

        "#EXT-X-STREAM-INF:BANDWIDTH=2560000,AVERAGE-BANDWIDTH=2000000\n"
        "http://example.com/mid.m3u8\n"

        "#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=6000000,CODECS=\"\",BANDWIDTH=7680000\n"
        "http://example.com/hi.m3u8\n"

        "#EXT-X-STREAM-INF:BANDWIDTH=65000,CODECS=\"mp4a.40.5\"\n"
        "http://example.com/audio-only.m3u8\n"

        "#EXT-X-STREAM-INF:CODECS=\"avc1.4d401e,mp4a.40.2\",BANDWIDTH=7680000\n"
        "http://example.com/higher.m3u8\n";
    enum HLSPlaylistType playlist_type = hls_parse_playlist(playlist,
                                                            data,
                                                            strlen(data));
    ASSERT_EQ(tr, playlist_type, HLS_MASTER_PLAYLIST);

    size_t n_streams = hls_get_variant_streams(&streams, playlist);

    ASSERT_EQ(tr, n_streams, 5);

    ASSERT_STR_EQ(tr, streams[0].url, "http://example.com/low.m3u8");
    ASSERT_EQ(tr, streams[0].num_codecs, 1);
    ASSERT_STR_EQ(tr, streams[0].codecs[0].name, "");
    ASSERT_EQ(tr, streams[0].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_EQ(tr, *(streams[0].bandwidth), 1280000);

    ASSERT_STR_EQ(tr, streams[1].url, "http://example.com/mid.m3u8");
    ASSERT_EQ(tr, streams[1].num_codecs, 1);
    ASSERT_STR_EQ(tr, streams[1].codecs[0].name, "");
    ASSERT_EQ(tr, streams[0].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_EQ(tr, *(streams[1].bandwidth), 2560000);

    ASSERT_STR_EQ(tr, streams[2].url, "http://example.com/hi.m3u8");
    ASSERT_EQ(tr, streams[2].num_codecs, 1);
    ASSERT_STR_EQ(tr, streams[2].codecs[0].name, "");
    ASSERT_EQ(tr, *(streams[2].bandwidth), 7680000);

    ASSERT_STR_EQ(tr, streams[3].url, "http://example.com/audio-only.m3u8");
    ASSERT_EQ(tr, *(streams[3].bandwidth), 65000);
    ASSERT_EQ(tr, streams[3].num_codecs, 1);
    ASSERT_STR_EQ(tr, streams[3].codecs[0].name, "mp4a.40.5");
    ASSERT_EQ(tr, streams[3].codecs[0].codec_media_type, SC_CODEC_AUDIO);

    ASSERT_STR_EQ(tr, streams[4].url, "http://example.com/higher.m3u8");
    ASSERT_EQ(tr, *(streams[4].bandwidth), 7680000);
    ASSERT_EQ(tr, streams[4].num_codecs, 2);
    ASSERT_STR_EQ(tr, streams[4].codecs[0].name, "avc1.4d401e");
    ASSERT_EQ(tr, streams[4].codecs[0].codec_media_type, SC_CODEC_VIDEO);
    ASSERT_STR_EQ(tr, streams[4].codecs[1].name, "mp4a.40.2");
    ASSERT_EQ(tr, streams[4].codecs[1].codec_media_type, SC_CODEC_AUDIO);

    hls_playlist_free(playlist);
}

void test_hls_parse_whitespace_in_codec(struct TestResult *tr) {
    HLSPlaylist *playlist = hls_playlist_new("http://example.com/master.m3u8");
    struct HLSVariantStream *streams = NULL;
    const char *data =
        "#EXTM3U\n"
        "#EXT-X-STREAM-INF:CODECS=\"avc1.4d401e, mp4a.40.2\",BANDWIDTH=7680000\n"
        "http://example.com/video.m3u8\n";

    hls_parse_playlist(playlist, data, strlen(data));
    hls_get_variant_streams(&streams, playlist);

    ASSERT_STR_EQ(tr, streams[0].url, "http://example.com/video.m3u8");
    ASSERT_EQ(tr, streams[0].num_codecs, 2);
    ASSERT_STR_EQ(tr, streams[0].codecs[0].name, "avc1.4d401e");
    ASSERT_EQ(tr, streams[0].codecs[0].codec_media_type, SC_CODEC_VIDEO);
    ASSERT_STR_EQ(tr, streams[0].codecs[1].name, "mp4a.40.2");
    ASSERT_EQ(tr, streams[0].codecs[1].codec_media_type, SC_CODEC_AUDIO);

    hls_playlist_free(playlist);
}

void test_hls_parse_media_playlist(struct TestResult *tr) {
    HLSPlaylist *playlist = hls_playlist_new(
        "http://media.example.com/master.m3u8"
    );
    struct HLSMediaSegment *segment = NULL;
    const char *data =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:10\n"
        "#EXT-X-VERSION:3\n"
        "#EXTINF:9.009\n"
        "http://media.example.com/first.ts\n"
        "#EXTINF:9.0090,\n"
        "http://media.example.com/second.ts\n"
        "#EXTINF:303,Foo\n"
        "third.ts\n"
        "#EXT-X-ENDLIST\n";
    enum HLSPlaylistType playlist_type = hls_parse_playlist(playlist,
                                                            data,
                                                            strlen(data));
    ASSERT_EQ(tr, playlist_type, HLS_MEDIA_PLAYLIST);

    size_t n_media_segments = hls_media_segments_len(playlist);
    ASSERT_EQ(tr, n_media_segments, 3);

    uint64_t next_start = hls_get_media_segment(&segment, playlist, 0);
    ASSERT_STR_EQ(tr, segment->url, "http://media.example.com/first.ts");
    ASSERT_EQ(tr, next_start, 9009);
    ASSERT_EQ(tr, segment->duration, 9009);

    hls_get_media_segment(&segment, playlist, next_start);
    ASSERT_STR_EQ(tr, segment->url, "http://media.example.com/second.ts");
    ASSERT_EQ(tr, segment->duration, 9009);
    next_start = hls_get_media_segment(&segment, playlist, next_start + 1000);
    ASSERT_STR_EQ(tr, segment->url, "http://media.example.com/second.ts");
    ASSERT_EQ(tr, segment->duration, 9009);
    ASSERT_EQ(tr, next_start, 18018);

    next_start = hls_get_media_segment(&segment, playlist, next_start);
    ASSERT_STR_EQ(tr, segment->url, "http://media.example.com/third.ts");
    ASSERT_EQ(tr, segment->duration, 303000);
    ASSERT_EQ(tr, next_start, 321018);

    next_start = hls_get_media_segment(&segment, playlist, next_start);
    ASSERT_EQ(tr, segment, NULL);
    ASSERT_EQ(tr, next_start, 0);

    hls_playlist_free(playlist);
}
