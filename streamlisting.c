#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hls.h"
#include "http.h"
#include "mpd.h"
#include "streamlisting.h"
#include "string.h"

static enum SCErrorCode get_hls_streams(struct SCStreamList **streams,
                                        char *manifest,
                                        size_t manifest_len,
                                        const char *manifest_url) {
    struct HLSVariantStream *hls_streams = NULL;
    HLSPlaylist *playlist = hls_playlist_new();

    hls_parse_playlist(playlist, manifest, manifest_len);
    (*streams)->len = hls_get_variant_streams(&hls_streams, playlist);
    (*streams)->streams = malloc((*streams)->len * sizeof (*streams)->streams[0]);
    for (size_t i = 0; i < (*streams)->len; i++) {
        struct SCStream *strm = &(*streams)->streams[i];

        strm->protocol = SC_PROTOCOL_HLS;
        strm->url = urljoin(manifest_url, hls_streams[i].url);
        strm->bitrate = *hls_streams[i].bandwidth;
        strm->num_codecs = hls_streams[i].num_codecs;
        strm->codecs = hls_streams[i].codecs;
        strm->id = malloc(i % 10 + 2);
        sprintf(strm->id, "%zu", i + 1);
    }
    (*streams)->private = playlist;

    return SC_SUCCESS;
}

struct MPDStreamsPrivate {
    struct MPD *mpd;
    struct Representation *representations;
};

static enum SCErrorCode get_mpd_streams(struct SCStreamList **streams,
                                        char *manifest,
                                        size_t manifest_len,
                                        const char *manifest_url) {
    (void)manifest_len;
    struct Representation *representations = NULL;

    struct MPD *mpd = mpd_parse(manifest, manifest_url);
    (*streams)->len = mpd_get_representations(&representations, mpd);
    (*streams)->streams = malloc((*streams)->len * sizeof (*streams)->streams[0]);
    for (size_t i = 0; i < (*streams)->len; i++) {
        struct SCStream *strm = &(*streams)->streams[i];

        strm->protocol = SC_PROTOCOL_MPD;
        strm->url = manifest_url;
        strm->bitrate = representations[i].bandwidth;
        strm->num_codecs = representations[i].num_codecs;
        strm->codecs = representations[i].codecs;
        strm->id = malloc(i % 10 + 2);
        sprintf(strm->id, "%zu", i + 1);
    }
    struct MPDStreamsPrivate *priv = malloc(sizeof priv[0]);
    priv->mpd = mpd;
    priv->representations = representations;
    (*streams)->private = priv;

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
        return get_hls_streams(streams, manifest, manifest_size, manifest_url);
    } else if (str_starts_with(manifest, "<MPD")) {
        return get_mpd_streams(streams, manifest, manifest_size, manifest_url);
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
            struct MPDStreamsPrivate *priv = streams->private;
            free(priv->representations);
            mpd_free(priv->mpd);
            free(priv);
        }
    }
    free(streams->streams);
    free(streams);
}
