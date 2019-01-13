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

#include "../src/streamlisting.h"

void test_get_streams_of_hls_master_playlist(struct TestResult *tr) {
    struct SCStreamList *streams = NULL;
    char *manifest =
        "#EXTM3U\n"

        "#EXT-X-STREAM-INF:BANDWIDTH=1280000,AVERAGE-BANDWIDTH=1000000\n"
        "low.m3u8\n"

        "#EXT-X-STREAM-INF:BANDWIDTH=2560000,AVERAGE-BANDWIDTH=2000000\n"
        "mid.m3u8\n"

        "#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=6000000,CODECS=\"\",BANDWIDTH=7680000\n"
        "hi.m3u8\n"

        "#EXT-X-STREAM-INF:BANDWIDTH=65000,CODECS=\"mp4a.40.5\"\n"
        "audio-only.m3u8\n"

        "#EXT-X-STREAM-INF:CODECS=\"avc1.4d401e,mp4a.40.2\",BANDWIDTH=7680000\n"
        "higher.m3u8\n";

    enum SCErrorCode ret = sc_get_streams(&streams,
                                          manifest,
                                          strlen(manifest),
                                          "http://example.com/master.m3u8");

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

void test_get_streams_of_hls_media_playlist(struct TestResult *tr) {
    struct SCStreamList *streams = NULL;
    char *manifest =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:10\n"
        "#EXT-X-VERSION:3\n"

        "#EXTINF:9.009,\n"
        "first.ts\n"

        "#EXTINF:9.009,\n"
        "second.ts\n"

        "#EXTINF:3.003,\n"
        "third.ts\n"

        "#EXT-X-ENDLIST";

    enum SCErrorCode ret = sc_get_streams(&streams,
                                          manifest,
                                          strlen(manifest),
                                          "http://example.com/master.m3u8");

    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_EQ(tr, streams->len, 1);

    ASSERT_EQ(tr, streams->streams[0].protocol, SC_PROTOCOL_HLS);
    ASSERT_STR_EQ(tr, streams->streams[0].url, "http://example.com/master.m3u8");
    ASSERT_STR_EQ(tr, streams->streams[0].id, "1");
    ASSERT_EQ(tr, streams->streams[0].bitrate, 0);
    ASSERT_EQ(tr, streams->streams[0].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[0].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_STR_EQ(tr, streams->streams[0].codecs[0].name, "");

    sc_streams_free(streams);
}

void test_get_segments_of_hls_stream(struct TestResult *tr) {
    struct SCStreamSegmentData *segment_data = NULL;
    struct SCStream stream = {
        SC_PROTOCOL_HLS,
        "http://example.com/master.m3u8",
        "1",
        0,
        0,
        NULL
    };
    char *manifest =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:10\n"
        "#EXT-X-VERSION:3\n"

        "#EXTINF:9.009,\n"
        "first.ts\n"

        "#EXTINF:9.009,\n"
        "http://example.com/second.ts\n"

        "#EXTINF:3.003,\n"
        "third.ts\n"

        "#EXT-X-ENDLIST";

    enum SCErrorCode ret = sc_get_stream_segment_data(&segment_data,
                                                      &stream,
                                                      manifest,
                                                      strlen(manifest));

    ASSERT_EQ(tr, ret, SC_SUCCESS);

    ASSERT_EQ(tr, segment_data->num_segments, 3);

    struct SCStreamSegment segment;
    uint64_t time = 0;

    ret = sc_get_stream_segment(&segment, segment_data, SC_INITIALIZATION_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_EQ(tr, segment.url, NULL);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/first.ts");
    ASSERT_EQ(tr, segment.duration, 9009);
    ASSERT_EQ(tr, time, 9009);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/second.ts");
    ASSERT_EQ(tr, segment.duration, 9009);
    ASSERT_EQ(tr, time, 18018);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/third.ts");
    ASSERT_EQ(tr, segment.duration, 3003);
    ASSERT_EQ(tr, time, 21021);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_EQ(tr, segment.url, NULL);
    ASSERT_EQ(tr, segment.duration, 0);
    ASSERT_EQ(tr, time, 0);
    sc_stream_segment_free(&segment, stream.protocol);

    sc_stream_segment_data_free(segment_data);
}

void test_mpd_streamlist_parse_manifest(struct TestResult *tr) {
    struct SCStreamList *streams = NULL;
    // clang-format off
    char *manifest =
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd\" profiles=\"urn:mpeg:dash:profile:isoff-live:2011\" type=\"static\" publishTime=\"2018-08-01T02:44:03Z\" mediaPresentationDuration=\"PT58M29.16S\" minBufferTime=\"PT1.5S\">"
        "  <ProgramInformation>"
        "    <Title>Title</Title>"
        "  </ProgramInformation>"
        "  <Period id=\"0\" start=\"PT0S\">"
        "    <AdaptationSet id=\"0\" mimeType=\"video/mp4\" segmentAlignment=\"true\" frameRate=\"25\" startWithSAP=\"1\" group=\"1\" par=\"16:9\" maxWidth=\"1280\" maxHeight=\"720\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
        "      <SegmentTemplate initialization=\"video_$RepresentationID$_init.m4s\" media=\"video_$RepresentationID$_t$Time$.m4s\" timescale=\"90000\" presentationTimeOffset=\"0\">"
        "        <SegmentTimeline>"
        "          <S t=\"0\" d=\"900000\"/>"
        "          <S d=\"900000\" r=\"2\"/>"
        "          <S d=\"824400\"/>"
        "        </SegmentTimeline>"
        "      </SegmentTemplate>"
        "      <Representation id=\"repr1\" bandwidth=\"1583896\" width=\"1280\" height=\"720\" codecs=\"avc1.4d401f\" sar=\"1:1\"/>"
        "      <Representation id=\"repr2\" bandwidth=\"143992\" width=\"512\" height=\"288\" codecs=\"avc1.42c015\" sar=\"1:1\"/>"
        "      <Representation id=\"repr3\" bandwidth=\"891903\" width=\"768\" height=\"432\" sar=\"1:1\"/>"
        "    </AdaptationSet>"
        "    <AdaptationSet id=\"1\" mimeType=\"audio/mp4\" segmentAlignment=\"true\" startWithSAP=\"1\" group=\"2\" lang=\"sv\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
        "      <AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\"/>"
        "      <Role schemeIdUri=\"urn:mpeg:dash:role:2011\" value=\"main\"/>"
        "      <SegmentTemplate initialization=\"chunk_ctaudio_rid$RepresentationID$_cinit_mpd.m4s\" media=\"chunk_ctaudio_rid$RepresentationID$_cs$Time$_mpd.m4s\" timescale=\"24000\" presentationTimeOffset=\"0\">"
        "        <SegmentTimeline>"
        "          <S t=\"0\" d=\"240000\"/>"
        "          <S d=\"240000\" r=\"2\"/>"
        "          <S d=\"219840\"/>"
        "        </SegmentTimeline>"
        "      </SegmentTemplate>"
        "      <Representation id=\"repr1\" bandwidth=\"94733\" codecs=\"mp4a.40.5\" audioSamplingRate=\"24000\"/>"
        "    </AdaptationSet>"
        "  </Period>"
        "</MPD>";
    // clang-format on

    enum SCErrorCode ret = sc_get_streams(&streams,
                                          manifest,
                                          strlen(manifest),
                                          "http://example.com/manifest.mpd");

    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_EQ(tr, streams->len, 4);

    ASSERT_EQ(tr, streams->streams[0].protocol, SC_PROTOCOL_MPD);
    ASSERT_STR_EQ(tr, streams->streams[0].url, "http://example.com/manifest.mpd");
    ASSERT_STR_EQ(tr, streams->streams[0].id, "1");
    ASSERT_EQ(tr, streams->streams[0].bitrate, 1583896);
    ASSERT_EQ(tr, streams->streams[0].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[0].codecs[0].codec_media_type, SC_CODEC_VIDEO);
    ASSERT_STR_EQ(tr, streams->streams[0].codecs[0].name, "avc1.4d401f");

    ASSERT_EQ(tr, streams->streams[1].protocol, SC_PROTOCOL_MPD);
    ASSERT_STR_EQ(tr, streams->streams[1].url, "http://example.com/manifest.mpd");
    ASSERT_STR_EQ(tr, streams->streams[1].id, "2");
    ASSERT_EQ(tr, streams->streams[1].bitrate, 143992);
    ASSERT_EQ(tr, streams->streams[1].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[1].codecs[0].codec_media_type, SC_CODEC_VIDEO);
    ASSERT_STR_EQ(tr, streams->streams[1].codecs[0].name, "avc1.42c015");

    ASSERT_EQ(tr, streams->streams[2].protocol, SC_PROTOCOL_MPD);
    ASSERT_STR_EQ(tr, streams->streams[2].url, "http://example.com/manifest.mpd");
    ASSERT_STR_EQ(tr, streams->streams[2].id, "3");
    ASSERT_EQ(tr, streams->streams[2].bitrate, 891903);
    ASSERT_EQ(tr, streams->streams[2].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[2].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_STR_EQ(tr, streams->streams[2].codecs[0].name, "");

    ASSERT_EQ(tr, streams->streams[3].protocol, SC_PROTOCOL_MPD);
    ASSERT_STR_EQ(tr, streams->streams[3].url, "http://example.com/manifest.mpd");
    ASSERT_STR_EQ(tr, streams->streams[3].id, "4");
    ASSERT_EQ(tr, streams->streams[3].bitrate, 94733);
    ASSERT_EQ(tr, streams->streams[3].num_codecs, 1);
    ASSERT_EQ(tr, streams->streams[3].codecs[0].codec_media_type, SC_CODEC_AUDIO);
    ASSERT_STR_EQ(tr, streams->streams[3].codecs[0].name, "mp4a.40.5");

    sc_streams_free(streams);
}

void test_get_segments_of_mpd_stream(struct TestResult *tr) {
    struct SCStreamSegmentData *segment_data = NULL;
    struct SCStream stream = {
        .protocol = SC_PROTOCOL_MPD,
        .url = "http://example.com/manifest.mpd",
        .id = "4",
        .bitrate = 0,
        .num_codecs = 0,
        .codecs = NULL
    };
    // clang-format off
    char *manifest =
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd\" profiles=\"urn:mpeg:dash:profile:isoff-live:2011\" type=\"static\" publishTime=\"2018-08-01T02:44:03Z\" mediaPresentationDuration=\"PT58M29.16S\" minBufferTime=\"PT1.5S\">"
        "  <ProgramInformation>"
        "    <Title>Title</Title>"
        "  </ProgramInformation>"
        "  <Period id=\"0\" start=\"PT0S\">"
        "    <AdaptationSet id=\"0\" mimeType=\"video/mp4\" segmentAlignment=\"true\" frameRate=\"25\" startWithSAP=\"1\" group=\"1\" par=\"16:9\" maxWidth=\"1280\" maxHeight=\"720\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
        "      <SegmentTemplate initialization=\"video_$RepresentationID$_init.m4s\" media=\"video_$RepresentationID$_t$Time$.m4s\" timescale=\"90000\" presentationTimeOffset=\"0\">"
        "        <SegmentTimeline>"
        "          <S t=\"0\" d=\"900000\"/>"
        "          <S d=\"900000\" r=\"2\"/>"
        "          <S d=\"824400\"/>"
        "        </SegmentTimeline>"
        "      </SegmentTemplate>"
        "      <Representation id=\"repr1\" bandwidth=\"1583896\" width=\"1280\" height=\"720\" codecs=\"avc1.4d401f\" sar=\"1:1\"/>"
        "      <Representation id=\"repr2\" bandwidth=\"143992\" width=\"512\" height=\"288\" codecs=\"avc1.42c015\" sar=\"1:1\"/>"
        "      <Representation id=\"repr3\" bandwidth=\"891903\" width=\"768\" height=\"432\" sar=\"1:1\"/>"
        "    </AdaptationSet>"
        "    <AdaptationSet id=\"1\" mimeType=\"audio/mp4\" segmentAlignment=\"true\" startWithSAP=\"1\" group=\"2\" lang=\"sv\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
        "      <AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\"/>"
        "      <Role schemeIdUri=\"urn:mpeg:dash:role:2011\" value=\"main\"/>"
        "      <SegmentTemplate initialization=\"chunk_ctaudio_rid$RepresentationID$_cinit_mpd.m4s\" media=\"chunk_ctaudio_rid$RepresentationID$_cs$Time$_mpd.m4s\" timescale=\"24000\" presentationTimeOffset=\"0\">"
        "        <SegmentTimeline>"
        "          <S t=\"0\" d=\"240000\"/>"
        "          <S d=\"240000\" r=\"2\"/>"
        "          <S d=\"219840\"/>"
        "        </SegmentTimeline>"
        "      </SegmentTemplate>"
        "      <Representation id=\"repr1\" bandwidth=\"94733\" codecs=\"mp4a.40.5\" audioSamplingRate=\"24000\"/>"
        "    </AdaptationSet>"
        "  </Period>"
        "</MPD>";
    // clang-format on

    enum SCErrorCode ret = sc_get_stream_segment_data(&segment_data,
                                                      &stream,
                                                      manifest,
                                                      strlen(manifest));

    ASSERT_EQ(tr, ret, SC_SUCCESS);

    ASSERT_EQ(tr, segment_data->num_segments, 5);

    struct SCStreamSegment segment;
    uint64_t time = 0;

    ret = sc_get_stream_segment(&segment, segment_data, SC_INITIALIZATION_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/chunk_ctaudio_ridrepr1_cinit_mpd.m4s");
    ASSERT_EQ(tr, time, 0);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/chunk_ctaudio_ridrepr1_cs0_mpd.m4s");
    ASSERT_EQ(tr, segment.duration, 240000);
    ASSERT_EQ(tr, time, 240000);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/chunk_ctaudio_ridrepr1_cs240000_mpd.m4s");
    ASSERT_EQ(tr, segment.duration, 240000);
    ASSERT_EQ(tr, time, 480000);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/chunk_ctaudio_ridrepr1_cs480000_mpd.m4s");
    ASSERT_EQ(tr, segment.duration, 240000);
    ASSERT_EQ(tr, time, 720000);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/chunk_ctaudio_ridrepr1_cs720000_mpd.m4s");
    ASSERT_EQ(tr, segment.duration, 240000);
    ASSERT_EQ(tr, time, 960000);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_STR_EQ(tr, segment.url, "http://example.com/chunk_ctaudio_ridrepr1_cs960000_mpd.m4s");
    ASSERT_EQ(tr, segment.duration, 219840);
    ASSERT_EQ(tr, time, 1179840);
    sc_stream_segment_free(&segment, stream.protocol);

    ret = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time);
    ASSERT_EQ(tr, ret, SC_SUCCESS);
    ASSERT_EQ(tr, segment.url, NULL);
    ASSERT_EQ(tr, segment.duration, 0);
    ASSERT_EQ(tr, time, 0);
    sc_stream_segment_free(&segment, stream.protocol);

    sc_stream_segment_data_free(segment_data);
}
