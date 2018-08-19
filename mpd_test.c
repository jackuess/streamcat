#include "mpd.h"
#include "minunit.h"

extern char *test_mpd_manifest_parse()
{
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
                           "      <Representation id=\"repr7\" bandwidth=\"891903\" width=\"768\" height=\"432\" codecs=\"avc1.4d401e\" sar=\"1:1\"/>"
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
                           "      <Representation id=\"repr8\" bandwidth=\"94733\" codecs=\"mp4a.40.5\" audioSamplingRate=\"24000\"/>"
                           "    </AdaptationSet>"
                           "  </Period>"
                           "</MPD>";
    struct MPD *mpd = mpd_parse(manifest);
    const struct Representation **representations = mpd_get_representations(mpd);

    ASSERT_STR_EQ(representations[0]->mime_type, "video/mp4");
    ASSERT_STR_EQ(representations[1]->mime_type, "video/mp4");
    ASSERT_STR_EQ(representations[2]->mime_type, "video/mp4");
    ASSERT_STR_EQ(representations[3]->mime_type, "video/mp4");
    ASSERT_STR_EQ(representations[4]->mime_type, "video/mp4");
    ASSERT_STR_EQ(representations[6]->mime_type, "video/mp4");
    ASSERT_STR_EQ(representations[7]->mime_type, "audio/mp4");
    ASSERT_TRUE("Expected no more than 8 representations", representations[8] == NULL);

    char *url;
    long next_start = 0;

    mpd_get_url(&url, "http://foo.bar/", representations[3], INITIALIZATION_URL, 0);
    ASSERT_STR_EQ(url, "http://foo.bar/video_repr4_init.m4s");

    next_start = mpd_get_url(&url, "http://foo.bar/", representations[3], MEDIA_URL, 0);
    ASSERT_EQ(next_start, 900000);
    ASSERT_STR_EQ(url, "http://foo.bar/video_repr4_t0.m4s");
    next_start = mpd_get_url(&url, "http://foo.bar/", representations[3], MEDIA_URL, next_start);
    ASSERT_EQ(next_start, 1800000);
    ASSERT_STR_EQ(url, "http://foo.bar/video_repr4_t900000.m4s");
    next_start = mpd_get_url(&url, "http://foo.bar/", representations[2], MEDIA_URL, next_start);
    ASSERT_EQ(next_start, 2700000);
    ASSERT_STR_EQ(url, "http://foo.bar/video_repr3_t1800000.m4s");
    next_start = mpd_get_url(&url, "http://foo.bar/", representations[1], MEDIA_URL, next_start);
    ASSERT_EQ(next_start, 3600000);
    ASSERT_STR_EQ(url, "http://foo.bar/video_repr2_t2700000.m4s");
    next_start = mpd_get_url(&url, "http://foo.bar/", representations[0], MEDIA_URL, next_start);
    ASSERT_EQ(next_start, 4424400);
    ASSERT_STR_EQ(url, "http://foo.bar/video_repr1_t3600000.m4s");
    next_start = mpd_get_url(&url, "http://foo.bar/", representations[1], MEDIA_URL, next_start);
    ASSERT_EQ(next_start, 0);

    return NULL;
}
