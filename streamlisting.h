#ifndef streamlisting_h_INCLUDED
#define streamlisting_h_INCLUDED

#include <stdint.h>
#include <stddef.h>

#include "codec.h"

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
                           const struct SCStream *stream,
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

#endif // streamlisting_h_INCLUDED
