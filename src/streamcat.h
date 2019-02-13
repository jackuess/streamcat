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

#ifndef streamcat_h_INCLUDED
#define streamcat_h_INCLUDED

#include <stdint.h>
#include <stddef.h>

enum SCErrorCode {
    SC_SUCCESS,
    SC_UNKNOW_FORMAT,
    SC_OUT_OF_MEMORY
};
enum SCStreamProtocol {
    SC_PROTOCOL_HLS,
    SC_PROTOCOL_MPD
};

enum SCURLType { SC_INITIALIZATION_URL, SC_MEDIA_URL };

enum SCCodecMediaType {
    SC_CODEC_AUDIO,
    SC_CODEC_VIDEO,
    SC_CODEC_UNKNOWN
};

struct SCCodec {
    const char *name;
    enum SCCodecMediaType codec_media_type;
};

struct SCStream {
    enum SCStreamProtocol protocol;
    const char *url;
    char *id;
    uint64_t bitrate;
    size_t num_codecs;
    const struct SCCodec *codecs;
};
struct SCStreamList {
    struct SCStream *streams;
    size_t len;
    void *private;
};
struct SCStreamSegmentData {
    size_t num_segments;
    enum SCStreamProtocol protocol;
    void *private;
};
struct SCStreamSegment {
    const char *url;
    uint64_t duration;
};

enum SCErrorCode sc_get_streams(struct SCStreamList **streams,
                                char *manifest,
                                size_t manifest_size,
                                const char *manifest_url);
void sc_streams_free(struct SCStreamList *streams);
enum SCErrorCode
sc_get_stream_segment_data(struct SCStreamSegmentData **segment_data,
                           enum SCStreamProtocol stream_protocol,
                           const char *stream_id,
                           const char *stream_url,
                           char *manifest,
                           size_t manifest_size);
enum SCErrorCode
sc_get_stream_segment(struct SCStreamSegment *segment,
                      const struct SCStreamSegmentData *segment_data,
                      enum SCURLType url_type,
                      uint64_t *time);

void sc_stream_segment_free(struct SCStreamSegment *segment,
                            enum SCStreamProtocol protocol);

void sc_stream_segment_data_free(struct SCStreamSegmentData *segment_data);

#endif // streamcat_h_INCLUDED
