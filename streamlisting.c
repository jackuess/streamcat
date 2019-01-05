#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hls.h"
#include "http.h"
#include "mpd.h"
#include "streamlisting.h"
#include "string.h"

static enum SCErrorCode get_hls_streams(struct SCStreamList *streams,
                                        char *manifest,
                                        size_t manifest_len,
                                        const char *manifest_url) {
    struct HLSVariantStream *hls_streams = NULL;
    HLSPlaylist *playlist = hls_playlist_new(manifest_url);

    enum HLSPlaylistType type = hls_parse_playlist(playlist, manifest,
                                                   manifest_len);

    if (type == HLS_MASTER_PLAYLIST) {
        streams->len = hls_get_variant_streams(&hls_streams, playlist);
    } else if (type == HLS_MEDIA_PLAYLIST) {
        static struct SCCodec unknown_codec = {
            .codec_media_type = SC_CODEC_UNKNOWN,
            .name = ""
        };
        static uint64_t zero = 0;

        streams->len = 1;
        hls_streams = calloc(1, sizeof hls_streams[0]);
        hls_streams[0].url = manifest_url;
        hls_streams[0].bandwidth = &zero;
        hls_streams[0].num_codecs = 1;
        hls_streams[0].codecs = &unknown_codec;
    } else if (type == HLS_INVALID_PLAYLIST) {
        return SC_UNKNOW_FORMAT;
    }

    streams->streams = malloc(streams->len * sizeof streams->streams[0]);
    for (size_t i = 0; i < streams->len; i++) {
        struct SCStream *strm = &streams->streams[i];

        strm->protocol = SC_PROTOCOL_HLS;
        strm->url = urljoin(manifest_url, hls_streams[i].url);
        strm->bitrate = *hls_streams[i].bandwidth;
        strm->num_codecs = hls_streams[i].num_codecs;
        strm->codecs = hls_streams[i].codecs;
        strm->id = malloc(i % 10 + 2);
        sprintf(strm->id, "%zu", i + 1);
    }
    streams->private = playlist;

    if (type == HLS_MEDIA_PLAYLIST) {
        free(hls_streams);
    }

    return SC_SUCCESS;
}

struct MPDPrivate {
    struct MPD *mpd;
    struct Representation *representations;
    size_t curr_repr_idx;
};

static enum SCErrorCode get_mpd_streams(struct SCStreamList *streams,
                                        char *manifest,
                                        size_t manifest_len,
                                        const char *manifest_url) {
    (void)manifest_len;
    struct Representation *representations = NULL;

    struct MPD *mpd = mpd_parse(manifest, manifest_url);
    streams->len = mpd_get_representations(&representations, mpd);
    streams->streams = malloc(streams->len * sizeof streams->streams[0]);
    for (size_t i = 0; i < streams->len; i++) {
        struct SCStream *strm = &streams->streams[i];

        strm->protocol = SC_PROTOCOL_MPD;
        strm->url = manifest_url;
        strm->bitrate = representations[i].bandwidth;
        strm->num_codecs = representations[i].num_codecs;
        strm->codecs = representations[i].codecs;
        strm->id = malloc(i % 10 + 2);
        sprintf(strm->id, "%zu", i + 1);
    }
    struct MPDPrivate *priv = malloc(sizeof priv[0]);
    priv->mpd = mpd;
    priv->representations = representations;
    streams->private = priv;

    return SC_SUCCESS;
}

enum SCErrorCode sc_get_streams(struct SCStreamList **streams,
                                char *manifest,
                                size_t manifest_size,
                                const char *manifest_url) {
    *streams = malloc(sizeof *streams[0]);
    if (streams == NULL) {
        return SC_OUT_OF_MEMORY;
    }

    if (str_starts_with(manifest, "#EXTM3U")) {
        return get_hls_streams(*streams, manifest, manifest_size, manifest_url);
    } else if (str_starts_with(manifest, "<MPD")) {
        return get_mpd_streams(*streams, manifest, manifest_size, manifest_url);
    } else {
        return SC_UNKNOW_FORMAT;
    }
}

void sc_streams_free(struct SCStreamList *streams) {
    for (size_t i = 0; i < streams->len; i++) {
        free(streams->streams[i].id);
        if (streams->streams[i].protocol == SC_PROTOCOL_HLS) {
            free((char*)streams->streams[i].url);
        }
    }
    if (streams->private != NULL) {
        if (streams->streams[0].protocol == SC_PROTOCOL_HLS) {
            hls_playlist_free(streams->private);
        } else if (streams->streams[0].protocol == SC_PROTOCOL_MPD) {
            struct MPDPrivate *priv = streams->private;
            free(priv->representations);
            mpd_free(priv->mpd);
            free(priv);
        }
    }
    free(streams->streams);
    free(streams);
}

enum SCErrorCode get_hls_segment_data(struct SCStreamSegmentData *segment_data,
                                      const struct SCStream *stream,
                                      char *manifest,
                                      size_t manifest_size) {
    HLSPlaylist *playlist = hls_playlist_new(stream->url);
    hls_parse_playlist(playlist, manifest, manifest_size);
    segment_data->num_segments = hls_media_segments_len(playlist);
    segment_data->private = playlist;

    return SC_SUCCESS;
}

enum SCErrorCode get_mpd_segment_data(struct SCStreamSegmentData *segment_data,
                                      const struct SCStream *stream,
                                      char *manifest) {
    struct MPD *mpd = mpd_parse(manifest, stream->url);
    struct Representation *reprs = NULL;
    size_t num_reprs = mpd_get_representations(&reprs, mpd);
    size_t index;

    if (sscanf(stream->id, "%zu", &index) != 1 ||
          index > num_reprs) {
        return SC_UNKNOW_FORMAT;  // TODO(Jacques): Return a better error code
    }

    struct Representation *repr = &reprs[index - 1];
    segment_data->num_segments = mpd_get_url_count(repr);

    struct MPDPrivate *priv = malloc(sizeof priv[0]);
    priv->mpd = mpd;
    priv->representations = reprs;
    priv->curr_repr_idx = index - 1;
    segment_data->private = priv;

    return SC_SUCCESS;
}


enum SCErrorCode
sc_get_stream_segment_data(struct SCStreamSegmentData **segment_data,
                           const struct SCStream *stream,
                           char *manifest,
                           size_t manifest_size) {
    *segment_data = malloc(sizeof *segment_data[0]);
    if (segment_data == NULL) {
        return SC_OUT_OF_MEMORY;
    }

    (*segment_data)->private = NULL;
    (*segment_data)->protocol = stream->protocol;
    if (stream->protocol == SC_PROTOCOL_HLS) {
        return get_hls_segment_data(*segment_data, stream, manifest,
                                    manifest_size);
    } else if (stream->protocol == SC_PROTOCOL_MPD) {
        return get_mpd_segment_data(*segment_data, stream, manifest);
    } else {
        return SC_UNKNOW_FORMAT;
    }
}

void sc_stream_segment_free(struct SCStreamSegment *segment,
                            enum SCStreamProtocol protocol) {
    if (protocol == SC_PROTOCOL_MPD && segment->url != NULL) {
        free((char*)segment->url);
    }
}

void sc_stream_segment_data_free(struct SCStreamSegmentData *segment_data) {
    if (segment_data->private != NULL) {
        if (segment_data->protocol == SC_PROTOCOL_HLS) {
            hls_playlist_free(segment_data->private);
        } else if (segment_data->protocol == SC_PROTOCOL_MPD) {
            struct MPDPrivate *priv = segment_data->private;
            free(priv->representations);
            mpd_free(priv->mpd);
            free(priv);
        }
    }
    free(segment_data);
}

enum
SCErrorCode get_hls_stream_segment(struct SCStreamSegment *segment,
                                   const struct SCStreamSegmentData *segment_data,
                                   enum SCURLType url_type,
                                   uint64_t *time) {
    if (url_type == SC_INITIALIZATION_URL) {
        segment->url = NULL;
        segment->duration = 0;
        return SC_SUCCESS;
    }

    struct HLSMediaSegment *hls_segment = NULL;
    *time = hls_get_media_segment(&hls_segment,
                                  (HLSPlaylist*)segment_data->private,
                                  *time);
    if (hls_segment == NULL) {
        segment->duration = 0;
        segment->url = NULL;
    } else {
        segment->duration = hls_segment->duration;
        segment->url = hls_segment->url;
    }
    return SC_SUCCESS;
}

enum
SCErrorCode get_mpd_stream_segment(struct SCStreamSegment *segment,
                                   const struct SCStreamSegmentData *segment_data,
                                   enum SCURLType url_type,
                                   uint64_t *time) {
    struct MPDPrivate *priv = segment_data->private;
    struct Representation *repr = &priv->representations[priv->curr_repr_idx];
    char *url = NULL;
    uint64_t next_time = (uint64_t)mpd_get_url(&url, repr, url_type, *time);
    if (next_time > 0) {
        segment->duration = next_time - *time;
    } else {
        segment->duration = 0;
    }
    segment->url = url;
    if (url_type == SC_MEDIA_URL) {
        *time = next_time;
    }

    return SC_SUCCESS;
}

enum SCErrorCode
sc_get_stream_segment(struct SCStreamSegment *segment,
                      const struct SCStreamSegmentData *segment_data,
                      enum SCURLType url_type,
                      uint64_t *time) {
    if (segment_data->protocol == SC_PROTOCOL_HLS) {
        return get_hls_stream_segment(segment, segment_data, url_type, time);
    } else if (segment_data->protocol == SC_PROTOCOL_MPD) {
        return get_mpd_stream_segment(segment, segment_data, url_type, time);
    } else {
        return SC_UNKNOW_FORMAT;
    }
}
