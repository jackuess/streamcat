#ifndef hls_h_INCLUDED
#define hls_h_INCLUDED

#include <stddef.h>
#include <stdint.h>

typedef struct HLSPlaylist HLSPlaylist;

enum HLSPlaylistType {
    HLS_INVALID_PLAYLIST,
    HLS_MASTER_PLAYLIST,
    HLS_MEDIA_PLAYLIST
};

struct HLSVariantStream {
    uint64_t *bandwidth;
    const char *url;
};

struct HLSMediaSegment {
    uint64_t *duration;
    const char *url;
};

HLSPlaylist *hls_playlist_new();
enum HLSPlaylistType hls_parse_playlist(HLSPlaylist *playlist, const char *buffer,
                        size_t buffer_n);
size_t hls_get_media_segments(struct HLSMediaSegment **segments,
                              const HLSPlaylist *playlist);
void hls_media_segments_free(struct HLSMediaSegment *segments);
size_t hls_get_variant_streams(struct HLSVariantStream **streams,
                               const HLSPlaylist *playlist);
void hls_variant_streams_free(struct HLSVariantStream *streams);
void hls_playlist_free(HLSPlaylist *playlist);

#endif // hls_h_INCLUDED
