#ifndef mpd_h_INCLUDED
#define mpd_h_INCLUDED

#include <stddef.h>

#include "codec.h"
#include "streamlisting.h"

struct MPD;

struct SegmentTemplate {
    long start_number;
    const char *initialization;
    const char *media;
    struct SegmentTime *timeline;
    size_t timeline_refs;
};

struct Representation {
    long bandwidth;
    const char *id;
    const char *mime_type;
    const char *origin_url;
    struct SegmentTemplate segment_template;
    const struct SCCodec *codecs;
    size_t num_codecs;
    // SegmentBase
    // SegmentList
};

struct MPD *mpd_parse(const char *buffer, const char *origin_url);
void mpd_free(struct MPD *mpd);
size_t mpd_get_representations(struct Representation **repr,
                               const struct MPD *mpd);
size_t mpd_get_url_count(const struct Representation *repr);
long mpd_get_url(char **url,
                 const struct Representation *repr,
                 enum SCURLType url_type,
                 long time);

#endif  // mpd_h_INCLUDED
