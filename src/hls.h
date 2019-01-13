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

#ifndef hls_h_INCLUDED
#define hls_h_INCLUDED

#include <stddef.h>
#include <stdint.h>

#include "codec.h"

typedef struct HLSPlaylist HLSPlaylist;

enum HLSPlaylistType {
    HLS_INVALID_PLAYLIST,
    HLS_MASTER_PLAYLIST,
    HLS_MEDIA_PLAYLIST
};

struct HLSVariantStream {
    uint64_t *bandwidth;
    const char *url;
    struct SCCodec *codecs;
    size_t num_codecs;
};

struct HLSMediaSegment {
    uint64_t duration;
    size_t start_time;
    char *url;
};

HLSPlaylist *hls_playlist_new(const char *origin_url);
enum HLSPlaylistType hls_parse_playlist(HLSPlaylist *playlist, const char *buffer,
                                        size_t buffer_n);
const char* hls_playlist_get_origin_url(HLSPlaylist *playlist);
uint64_t hls_get_media_segment(struct HLSMediaSegment **segment,
                               const HLSPlaylist *playlist,
                               uint64_t start_time);
size_t hls_media_segments_len(const HLSPlaylist *playlist);
size_t hls_get_variant_streams(struct HLSVariantStream **streams,
                               const HLSPlaylist *playlist);
void hls_playlist_free(HLSPlaylist *playlist);

#endif // hls_h_INCLUDED
