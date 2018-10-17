#include <string.h>

#include "vendor/scut/scut.h"

#include "streamlisting.h"

void test_hls_streamlist_parse_master_playlist(struct TestResult *tr) {
    struct SCStreamList *streams = NULL;
    char *manifest =
        "#EXTM3U\n"

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

    enum SCErrorCode ret = sc_get_streams(&streams, manifest, strlen(manifest));

    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_EQ(tr, streams->len, 5);

    ASSERT_EQ(tr, streams->streams[0].protocol, SC_PROTOCOL_HLS);
    ASSERT_STR_EQ(tr, streams->streams[0].url, "http://example.com/low.m3u8");
    ASSERT_STR_EQ(tr, streams->streams[0].id, "1");
    ASSERT_EQ(tr, streams->streams[0].bitrate, 1280000);
    ASSERT_EQ(tr, streams->streams[0].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[0].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_STR_EQ(tr, streams->streams[0].codecs[0].name, "");

    ASSERT_EQ(tr, streams->streams[1].protocol, SC_PROTOCOL_HLS);
    ASSERT_STR_EQ(tr, streams->streams[1].url, "http://example.com/mid.m3u8");
    ASSERT_STR_EQ(tr, streams->streams[1].id, "2");
    ASSERT_EQ(tr, streams->streams[1].bitrate, 2560000);
    ASSERT_EQ(tr, streams->streams[1].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[1].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_STR_EQ(tr, streams->streams[1].codecs[0].name, "");

    ASSERT_EQ(tr, streams->streams[2].protocol, SC_PROTOCOL_HLS);
    ASSERT_STR_EQ(tr, streams->streams[2].url, "http://example.com/hi.m3u8");
    ASSERT_STR_EQ(tr, streams->streams[2].id, "3");
    ASSERT_EQ(tr, streams->streams[2].bitrate, 7680000);
    ASSERT_EQ(tr, streams->streams[2].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[2].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_STR_EQ(tr, streams->streams[2].codecs[0].name, "");

    ASSERT_EQ(tr, streams->streams[3].protocol, SC_PROTOCOL_HLS);
    ASSERT_STR_EQ(tr, streams->streams[3].url, "http://example.com/audio-only.m3u8");
    ASSERT_STR_EQ(tr, streams->streams[3].id, "4");
    ASSERT_EQ(tr, streams->streams[3].bitrate, 65000);
    ASSERT_EQ(tr, streams->streams[3].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[3].codecs[0].codec_media_type, SC_CODEC_AUDIO);
    ASSERT_STR_EQ(tr, streams->streams[3].codecs[0].name, "mp4a.40.5");

    ASSERT_EQ(tr, streams->streams[4].protocol, SC_PROTOCOL_HLS);
    ASSERT_STR_EQ(tr, streams->streams[4].url, "http://example.com/higher.m3u8");
    ASSERT_STR_EQ(tr, streams->streams[4].id, "5");
    ASSERT_EQ(tr, streams->streams[4].bitrate, 7680000);
    ASSERT_EQ(tr, streams->streams[4].num_codecs, 2);
    ASSERT_EQ(tr, streams->streams[4].codecs[0].codec_media_type, SC_CODEC_VIDEO);
    ASSERT_STR_EQ(tr, streams->streams[4].codecs[0].name, "avc1.4d401e");
    ASSERT_STR_EQ(tr, streams->streams[4].codecs[1].name, "mp4a.40.2");
    ASSERT_EQ(tr, streams->streams[4].codecs[1].codec_media_type, SC_CODEC_AUDIO);

    sc_streams_free(streams);
}
