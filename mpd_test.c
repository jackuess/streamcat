#include <stdlib.h>

#include "mpd.h"
#include "vendor/scut/scut.h"

void test_mpd_manifest_parse_time(struct TestResult *tr) {
    // clang-format off
    const char *manifest = "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd\" profiles=\"urn:mpeg:dash:profile:isoff-live:2011\" type=\"static\" publishTime=\"2018-08-01T02:44:03Z\" mediaPresentationDuration=\"PT58M29.16S\" minBufferTime=\"PT1.5S\">"
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
                           "      <Representation id=\"repr3\" bandwidth=\"2699800\" width=\"1280\" height=\"720\" codecs=\"avc1.64001f\" sar=\"1:1\"/>"
                           "      <Representation id=\"repr4\" bandwidth=\"251953\" width=\"512\" height=\"288\" codecs=\"avc1.42c015\" sar=\"1:1\"/>"
                           "      <Representation id=\"repr5\" bandwidth=\"359979\" width=\"512\" height=\"288\" codecs=\"avc1.42c015\" sar=\"1:1\"/>"
                           "      <Representation id=\"repr6\" bandwidth=\"539937\" width=\"512\" height=\"288\" codecs=\"avc1.42c015\" sar=\"1:1\"/>"
                           "      <Representation id=\"repr7\" bandwidth=\"891903\" width=\"768\" height=\"432\" sar=\"1:1\"/>"
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
    struct MPD *mpd = mpd_parse(manifest, "http://foo.bar/manifest.mpd");
    struct Representation *representations = NULL;
    size_t n_representations = mpd_get_representations(&representations, mpd);

    ASSERT_TRUE(tr, n_representations == 8);
    ASSERT_STR_EQ(tr, representations[0].mime_type, "video/mp4");
    ASSERT_EQ(tr, representations[0].num_codecs, 1);
    ASSERT_EQ(tr, representations[0].codecs[0].codec_media_type, SC_CODEC_VIDEO);
    ASSERT_STR_EQ(tr, representations[0].codecs[0].name, "avc1.4d401f");
    ASSERT_STR_EQ(tr, representations[1].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[2].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[3].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[4].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[6].mime_type, "video/mp4");
    ASSERT_EQ(tr, representations[6].num_codecs, 1);
    ASSERT_EQ(tr, representations[6].codecs[0].codec_media_type, SC_CODEC_UNKNOWN);
    ASSERT_STR_EQ(tr, representations[6].codecs[0].name, "");
    ASSERT_STR_EQ(tr, representations[7].mime_type, "audio/mp4");
    ASSERT_EQ(tr, representations[7].num_codecs, 1);
    ASSERT_EQ(tr, representations[7].codecs[0].codec_media_type, SC_CODEC_AUDIO);
    ASSERT_STR_EQ(tr, representations[7].codecs[0].name, "mp4a.40.5");

    char *url;
    long next_start = 0;

    for (size_t i = 0; i < n_representations; i++) {
        ASSERT_EQ(tr, mpd_get_url_count(&representations[i]), 5);
    }

    mpd_get_url(
        &url, &representations[3], SC_INITIALIZATION_URL, 0);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/video_repr4_init.m4s");
    free(url);

    next_start =
        mpd_get_url(&url, &representations[3], SC_MEDIA_URL, 0);
    ASSERT_EQ(tr, next_start, 900000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/video_repr4_t0.m4s");
    free(url);
    next_start = mpd_get_url(
        &url, &representations[3], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 1800000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/video_repr4_t900000.m4s");
    free(url);
    next_start = mpd_get_url(
        &url, &representations[2], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 2700000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/video_repr3_t1800000.m4s");
    free(url);
    next_start = mpd_get_url(
        &url, &representations[1], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 3600000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/video_repr2_t2700000.m4s");
    free(url);
    next_start = mpd_get_url(
        &url, &representations[0], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 4424400);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/video_repr1_t3600000.m4s");
    free(url);
    next_start = mpd_get_url(
        &url, &representations[1], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 0);

    free(representations);
    mpd_free(mpd);
}

void test_mpd_manifest_parse_numbers(struct TestResult *tr) {
    // clang-format off
    const char *manifest = "<MPD mediaPresentationDuration=\"PT3509.160S\" minBufferTime=\"PT2.00S\" profiles=\"urn:hbbtv:dash:profile:isoff-live:2012,urn:mpeg:dash:profile:isoff-live:2011\" type=\"static\" xmlns=\"urn:mpeg:dash:schema:mpd:2011\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 DASH-MPD.xsd\">"
                           "  <BaseURL>./</BaseURL>"
                           "  <Period>"
                           "    <AdaptationSet contentType=\"video\" mimeType=\"video/mp4\" par=\"16:9\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
                           "      <SegmentTemplate initialization=\"$RepresentationID$/$RepresentationID$_00000.m4v\" media=\"$RepresentationID$/$RepresentationID$_$Number%05d$.m4v\" startNumber=\"1\" timescale=\"25000\">"
                           "        <SegmentTimeline>"
                           "          <S d=\"150000\" t=\"0\" />"
                           "          <S d=\"150000\" r=\"2\" />"
                           "          <S d=\"129000\" />"
                           "        </SegmentTimeline>"
                           "      </SegmentTemplate>"
                           "      <Representation bandwidth=\"3228829\" codecs=\"avc1.64001f\" frameRate=\"25\" height=\"720\" id=\"repr1\" sar=\"1:1\" scanType=\"progressive\" width=\"1280\" />"
                           "      <Representation bandwidth=\"1043832\" codecs=\"avc1.4d401e\" frameRate=\"25\" height=\"432\" id=\"repr2\" sar=\"1:1\" scanType=\"progressive\" width=\"768\" />"
                           "      <Representation bandwidth=\"296852\" codecs=\"avc1.42c015\" frameRate=\"25\" height=\"288\" id=\"repr3\" sar=\"1:1\" scanType=\"progressive\" width=\"512\" />"
                           "    </AdaptationSet>"
                           "    <AdaptationSet contentType=\"audio\" mimeType=\"audio/mp4\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
                           "      <Accessibility schemeIdUri=\"urn:tva:metadata:cs:AudioPurposeCS:2007\" value=\"6\" />"
                           "      <Role schemeIdUri=\"urn:mpeg:dash:role:2011\" value=\"main\" />"
                           "      <SegmentTemplate initialization=\"$RepresentationID$/$RepresentationID$_00000.m4a\" media=\"$RepresentationID$/$RepresentationID$_$Number%05d$.m4a\" startNumber=\"1\" timescale=\"48000\">"
                           "        <SegmentTimeline>"
                           "          <S d=\"287744\" t=\"0\" />"
                           "          <S d=\"288768\" r=\"2\" />"
                           "          <S d=\"87552\" />"
                           "        </SegmentTimeline>"
                           "      </SegmentTemplate>"
                           "      <Representation audioSamplingRate=\"48000\" bandwidth=\"97393\" codecs=\"mp4a.40.5\" id=\"repr1\">"
                           "        <AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\" />"
                           "      </Representation>"
                           "      <Representation audioSamplingRate=\"48000\" bandwidth=\"97393\" codecs=\"mp4a.40.5\" id=\"repr2\">"
                           "        <AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\" />"
                           "      </Representation>"
                           "    </AdaptationSet>"
                           "  </Period>"
                           "  </MPD>";
    // clang-format on
    struct MPD *mpd = mpd_parse(manifest, "http://foo.bar/manifest.mpd");
    struct Representation *representations;
    size_t n_representations = mpd_get_representations(&representations, mpd);

    ASSERT_TRUE(tr, n_representations == 5);
    ASSERT_STR_EQ(tr, representations[0].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[1].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[2].mime_type, "video/mp4");
    ASSERT_STR_EQ(tr, representations[3].mime_type, "audio/mp4");
    ASSERT_STR_EQ(tr, representations[4].mime_type, "audio/mp4");

    char *url;
    long next_start = 0;

    for (size_t i = 0; i < n_representations; i++) {
        ASSERT_EQ(tr, mpd_get_url_count(&representations[i]), 5);
    }

    mpd_get_url(
        &url, &representations[2], SC_INITIALIZATION_URL, 0);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr3/repr3_00000.m4v");
    free(url);

    next_start =
        mpd_get_url(&url, &representations[2], SC_MEDIA_URL, 0);
    ASSERT_EQ(tr, next_start, 150000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr3/repr3_00001.m4v");
    free(url);

    next_start = mpd_get_url(
        &url, &representations[2], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 300000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr3/repr3_00002.m4v");
    free(url);

    next_start = mpd_get_url(
        &url, &representations[2], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 450000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr3/repr3_00003.m4v");
    free(url);

    next_start = mpd_get_url(
        &url, &representations[2], SC_MEDIA_URL, 300001);
    ASSERT_EQ(tr, next_start, 450000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr3/repr3_00003.m4v");
    free(url);
    next_start = mpd_get_url(
        &url, &representations[2], SC_MEDIA_URL, 449999);
    ASSERT_EQ(tr, next_start, 450000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr3/repr3_00003.m4v");
    free(url);

    next_start = mpd_get_url(
        &url, &representations[1], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 600000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr2/repr2_00004.m4v");
    free(url);

    next_start = mpd_get_url(
        &url, &representations[0], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 729000);
    ASSERT_STR_EQ(tr, url, "http://foo.bar/repr1/repr1_00005.m4v");
    free(url);

    next_start = mpd_get_url(
        &url, &representations[1], SC_MEDIA_URL, next_start);
    ASSERT_EQ(tr, next_start, 0);

    free(representations);
    mpd_free(mpd);
}
