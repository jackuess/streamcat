#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hls.h"
#include "streamlisting.h"

enum SCErrorCode sc_get_streams(struct SCStreamList **streams,
                                char *manifest,
                                size_t manifest_len) {
    struct HLSVariantStream *hls_streams = NULL;
    HLSPlaylist *playlist = hls_playlist_new();

    hls_parse_playlist(playlist, manifest, manifest_len);
    *streams = malloc(sizeof *streams[0]);
    (*streams)->len = hls_get_variant_streams(&hls_streams, playlist);
    (*streams)->streams = malloc((*streams)->len * sizeof (*streams)->streams[0]);
    for (size_t i = 0; i < (*streams)->len; i++) {
        struct SCStream *strm = &(*streams)->streams[i];

        strm->protocol = SC_PROTOCOL_HLS;
        strm->url = hls_streams[i].url;
        strm->bitrate = *hls_streams[i].bandwidth;
        strm->num_codecs = hls_streams[i].num_codecs;
        strm->codecs = hls_streams[i].codecs;
        strm->id = malloc(i % 10 + 2);
        sprintf(strm->id, "%zu", i + 1);
    }
    (*streams)->private = playlist;

    return SC_SUCCESS;
}

void sc_streams_free(struct SCStreamList *streams) {
    for (size_t i = 0; i < streams->len; i++) {
        free(streams->streams[i].id);
    }
    if (streams->private != NULL) {
        hls_playlist_free(streams->private);
    }
    free(streams->streams);
    free(streams);
}
