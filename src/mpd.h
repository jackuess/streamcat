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

#ifndef mpd_h_INCLUDED
#define mpd_h_INCLUDED

#include <stddef.h>

#include "codec.h"
#include "streamcat.h"

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
