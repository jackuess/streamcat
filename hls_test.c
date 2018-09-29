#include <string.h>

#include "vendor/scut/scut.h"

#include "hls.h"

void test_hls_parse_master_playlist(struct TestResult *tr) {
    HLSPlaylist *playlist = hls_playlist_new();
    struct HLSVariantStream *streams = NULL;
    const char *data =
        "#EXTM3U\n"
        "#A comment\n"
        "#\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=1280000,AVERAGE-BANDWIDTH=1000000\n"
        "http://example.com/low.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=2560000,AVERAGE-BANDWIDTH=2000000\n"
        "http://example.com/mid.m3u8\n"
        "#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=6000000,BANDWIDTH=7680000\n"
        "http://example.com/hi.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=65000,CODECS=\"mp4a.40.5\"\n"
        "http://example.com/audio-only.m3u8\n";
    enum HLSPlaylistType playlist_type = hls_parse_playlist(playlist,
                                                            data,
                                                            strlen(data));
    ASSERT_EQ(tr, playlist_type, HLS_MASTER_PLAYLIST);

    size_t n_streams = hls_get_variant_streams(&streams, playlist);

    ASSERT_EQ(tr, n_streams, 4);
    ASSERT_STR_EQ(tr, streams[0].url, "http://example.com/low.m3u8");
    ASSERT_EQ(tr, *(streams[0].bandwidth), 1280000);
    ASSERT_STR_EQ(tr, streams[1].url, "http://example.com/mid.m3u8");
    ASSERT_EQ(tr, *(streams[1].bandwidth), 2560000);
    ASSERT_STR_EQ(tr, streams[2].url, "http://example.com/hi.m3u8");
    ASSERT_EQ(tr, *(streams[2].bandwidth), 7680000);
    ASSERT_STR_EQ(tr, streams[3].url, "http://example.com/audio-only.m3u8");
    ASSERT_EQ(tr, *(streams[3].bandwidth), 65000);

    hls_variant_streams_free(streams);
    hls_playlist_free(playlist);
}

void test_hls_parse_media_playlist(struct TestResult *tr) {
    HLSPlaylist *playlist = hls_playlist_new();
    struct HLSMediaSegment *segment = NULL;
    const char *data =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:10\n"
        "#EXT-X-VERSION:3\n"
        "#EXTINF:9.009,\n"
        "http://media.example.com/first.ts\n"
        "#EXTINF:9.0090,\n"
        "http://media.example.com/second.ts\n"
        "#EXTINF:303,Foo\n"
        "http://media.example.com/third.ts\n"
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
    ASSERT_EQ(tr, next_start, 0);

    hls_playlist_free(playlist);
}
