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

enum CodecMediaType {
    CODEC_AUDIO,
    CODEC_VIDEO,
    CODEC_UNKNOWN
};

struct HLSCodec {
    const char *name;
    enum CodecMediaType codec_media_type;
};

struct HLSVariantStream {
    uint64_t *bandwidth;
    const char *url;
    struct HLSCodec *codecs;
    size_t num_codecs;
};

struct HLSMediaSegment {
    uint64_t duration;
    size_t start_time;
    const char *url;
};

HLSPlaylist *hls_playlist_new();
enum HLSPlaylistType hls_parse_playlist(HLSPlaylist *playlist, const char *buffer,
                        size_t buffer_n);
uint64_t hls_get_media_segment(struct HLSMediaSegment **segment,
                               const HLSPlaylist *playlist,
                               uint64_t start_time);
size_t hls_media_segments_len(const HLSPlaylist *playlist);
size_t hls_get_variant_streams(struct HLSVariantStream **streams,
                               const HLSPlaylist *playlist);
void hls_playlist_free(HLSPlaylist *playlist);

#endif // hls_h_INCLUDED
