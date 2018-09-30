#define _DEFAULT_SOURCE

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vendor/arr/arr.h"

#include "hls.h"

union HLSTagAttributeData {
    uint64_t integer;
    char *string;
};

enum HLSTagDataType {
    HLS_INTEGER_TAG,
    HLS_STRING_TAG
};

struct HLSTagAttribute {
    const char *name;
    enum HLSTagDataType data_type;
    union HLSTagAttributeData data;
};

enum HLSTagType {
    HLS_UNKNOWN_TAG,

    // Basic tags
    HLS_EXTM3U,
    HLS_EXT_X_VERSION,

    //  Media Segment Tags
    HLS_EXTINF,
    HLS_EXT_X_BYTERANGE,
    HLS_EXT_X_DISCONTINUITY,
    HLS_EXT_X_KEY,
    HLS_EXT_X_MAP,
    HLS_EXT_X_PROGRAM_DATE_TIME,
    HLS_EXT_X_DATERANGE,

    // Media Playlist Tags
    HLS_EXT_X_TARGETDURATION,
    HLS_EXT_X_MEDIA_SEQUENCE,
    HLS_EXT_X_DISCONTINUITY_SEQUENCE,
    HLS_EXT_X_ENDLIST,
    HLS_EXT_X_PLAYLIST_TYPE,
    HLS_EXT_X_I_FRAMES_ONLY,

    // Master Playlist Tags
    HLS_EXT_X_MEDIA,
    HLS_EXT_X_STREAM_INF,
    HLS_EXT_X_I_FRAME_STREAM_INF,
    HLS_EXT_X_SESSION_DATA,
    HLS_EXT_X_SESSION_KEY,

    // Media or Master Playlist Tags
    HLS_EXT_X_INDEPENDENT_SEGMENTS,
    HLS_EXT_X_START
};

struct HLSTag {
    enum HLSTagType type;
    struct HLSTagAttribute *attributes;  // TODO(Jacques): Use hashmap
};

struct HLSLine {
    bool is_tag;
    char *data;
    struct HLSTag *tag;
};

struct HLSPlaylist {
    char *data;
    struct HLSLine *lines;
    struct HLSMediaSegment *media_segments;
    struct HLSVariantStream *variant_streams;
};

HLSPlaylist *hls_playlist_new() {
    struct HLSPlaylist *playlist = calloc(1, sizeof *playlist);
    return playlist;
}

static enum HLSTagType tag_type(const char *name) {
    if (strcmp(name, "EXTM3U") == 0) {
        return HLS_EXTM3U;
    } else if (strcmp(name, "EXTINF") == 0) {
        return HLS_EXTINF;
    } else if (strcmp(name, "EXT-X-TARGETDURATION") == 0) {
        return HLS_EXT_X_TARGETDURATION;
    } else if (strcmp(name, "EXT-X-STREAM-INF") == 0) {
        return HLS_EXT_X_STREAM_INF;
    } else {
        return HLS_UNKNOWN_TAG;
    }
}

static void parse_tag_attributes(struct HLSTag *tag, char *data) {
    char *c = data;
    bool eol = false;

    while (!eol) {
        data = c;
        for (; *c != '=' && *c != '\0'; c++) {}
        if (*c == '\0') {
            break;
        }
        *c = '\0';
        struct HLSTagAttribute *attr = ARRAPPEND(&tag->attributes);
        attr->name = data;

        data = ++c;

        if (*c == '\0') {
            break;
        }
        char endtag_char = ',';
        if (*c == '"') {
            endtag_char = '"';
            c++;
        }

        for (; *c != endtag_char && *c != '\0'; c++) {}
        if (endtag_char == '"') {
            if (*c == '\0') {
                eol = true;
            } else {
                c++;
            }
        }
        if (*c == '\0') {
            eol = true;
        }
        *c = '\0';
        if (endtag_char == '"') {
            attr->data_type = HLS_STRING_TAG;
            data[strlen(data)-1] = '\0';
            attr->data.string = &data[1];
        } else {
            attr->data_type = HLS_INTEGER_TAG;
            attr->data.integer = strtoul(data, NULL, 10);
        }
        c++;
    }
}

static void parse_tag_attributes_extinf(struct HLSTag *tag, char *data) {
    char *c = data;

    for (; *c != ',' && *c != '\0'; c++) {}
    if  (*c == '\0') {
        return;
    }
    *c = '\0';

    struct HLSTagAttribute *attr = ARRAPPEND(&tag->attributes);
    attr->name = "DURATION";
    attr->data_type = HLS_INTEGER_TAG;
    attr->data.integer = (uint64_t)(strtod(data, NULL) * 1000);
}

static struct HLSTag *hls_parse_tag(char *line) {
    struct HLSTag *tag = malloc(sizeof *tag);
    if (tag == NULL) {
        return NULL;
    }
    tag->attributes = arrnew(0, sizeof tag->attributes[0]);

    char *c = line;

    c = strchr(c, ':');
    if (c == NULL) {
        tag->type = tag_type(line);
        return tag;
    }
    *c = '\0';
    tag->type = tag_type(line);

    switch (tag->type) {
        case HLS_EXTINF:
            parse_tag_attributes_extinf(tag, &c[1]);
            break;
        default:
            parse_tag_attributes(tag, &c[1]);
    }

    return tag;
}

static uint64_t *hls_tag_get_attribute_uint64(struct HLSTag *tag, const char *attr) {
    for (size_t i = 0; i < arrlen(tag->attributes); i++) {
        if (tag->attributes[i].data_type == HLS_INTEGER_TAG &&
                strcmp(tag->attributes[i].name, attr) == 0) {
            return &tag->attributes[i].data.integer;
        }
    }
    return NULL;
}

static char *hls_tag_get_attribute_string(struct HLSTag *tag, const char *attr) {
    for (size_t i = 0; i < arrlen(tag->attributes); i++) {
        if (tag->attributes[i].data_type == HLS_STRING_TAG &&
                strcmp(tag->attributes[i].name, attr) == 0) {
            return tag->attributes[i].data.string;
        }
    }
    return NULL;
}

static struct HLSLine *hls_get_lines(char *data) {
    char *rest = data;
    char *token = NULL;
    struct HLSLine *lines = arrnew(0, sizeof lines[0]);

    while ((token = strsep(&rest, "\n"))) {
        if (token[0] == '\0') {
            continue;
        }

        struct HLSLine *line = ARRAPPEND(&lines);
        if (lines == NULL) {
            break;
        }
        line->is_tag = (token[0] == '#');
        line->data = token;
        if (line->is_tag) {
            line->tag = hls_parse_tag(&line->data[1]);
        } else {
            line->tag = NULL;
        }
    }

    return lines;
}

static struct HLSMediaSegment *parse_media_segments(const struct HLSLine *lines) {
    struct HLSMediaSegment *arr_segments = arrnew(0, sizeof arr_segments[0]);
    uint64_t *current_duration = NULL;
    size_t time = 0;

    for (size_t i = 0; i < arrlen(lines); i++) {
        if (lines[i].is_tag) {
            struct HLSTag *tag = lines[i].tag;
            if (tag->type == HLS_EXTINF) {
                current_duration = hls_tag_get_attribute_uint64(tag, "DURATION");
            }
        } else {
            struct HLSMediaSegment *segment = ARRAPPEND(&arr_segments);
            segment->url = lines[i].data;
            if (current_duration == NULL) {
                // TODO(Jacques): Handle this as an error case - segments must have a duration
                segment->duration = 0;
            } else {
                segment->duration = *current_duration;
            }
            segment->start_time = time;
            current_duration = NULL;
            time += segment->duration;
        }
    }

    return arr_segments;
}

static struct HLSVariantStream *parse_variant_streams(const struct HLSLine *lines) {
    struct HLSVariantStream *streams = arrnew(0, sizeof streams[0]);
    uint64_t *current_bandwidth = NULL;
    char *current_codec_string = NULL;

    for (size_t i = 0; i < arrlen(lines); i++) {
        if (lines[i].is_tag) {
            struct HLSTag *tag = lines[i].tag;
            if (tag->type == HLS_EXT_X_STREAM_INF) {
                current_bandwidth = hls_tag_get_attribute_uint64(tag, "BANDWIDTH");
                current_codec_string = hls_tag_get_attribute_string(tag, "CODECS");
            }
        } else {
            struct HLSVariantStream *stream = ARRAPPEND(&streams);
            stream->url = lines[i].data;
            stream->bandwidth = current_bandwidth;
            if (current_codec_string == NULL) {
                stream->codecs = parse_csv_codecs("");
            } else {
                stream->codecs = parse_csv_codecs(current_codec_string);
            }
            stream->num_codecs = arrlen(stream->codecs);
            current_bandwidth = NULL;
            current_codec_string = NULL;
        }
    }

    return streams;
}

enum HLSPlaylistType hls_parse_playlist(HLSPlaylist *playlist,
                                        const char *buffer,
                                        size_t buffer_n) {
    playlist->data = strndup(buffer, buffer_n);
    playlist->lines = hls_get_lines(playlist->data);
    if (playlist->lines == NULL) {
        return HLS_INVALID_PLAYLIST;
    }

    for (size_t i = 0; i < arrlen(playlist->lines); i++) {
        if (playlist->lines[i].is_tag) {
            if (playlist->lines[i].tag->type == HLS_EXT_X_TARGETDURATION) {
                playlist->media_segments = parse_media_segments(playlist->lines);
                return HLS_MEDIA_PLAYLIST;
            }
        }
    }

    playlist->variant_streams = parse_variant_streams(playlist->lines);
    return HLS_MASTER_PLAYLIST;
}

static int get_media_segment_time_collision(const void *key, const void *value) {
    uint64_t *time = (uint64_t *)key;
    struct HLSMediaSegment *segment = (struct HLSMediaSegment *)value;

    if (*time >= segment->start_time && *time < (segment->start_time + segment->duration)) {
        return 0;
    } else if (*time < segment->start_time) {
        return -1;
    } else {
        return 1;
    }
}

uint64_t hls_get_media_segment(struct HLSMediaSegment **segment,
                               const HLSPlaylist *playlist,
                               uint64_t start_time) {
    if (playlist->media_segments == NULL) {
        return 0;
    }
    size_t num_media_segments = arrlen(playlist->media_segments);
    *segment = bsearch(&start_time,
                       playlist->media_segments,
                       num_media_segments,
                       sizeof playlist->media_segments[0],
                       get_media_segment_time_collision);
    if (*segment == NULL) {
        return 0;
    } else if ((size_t)(*segment - playlist->media_segments + 1) == num_media_segments) {
        return 0;
    } else {
        return (*segment)->start_time + (*segment)->duration;
    }
}

size_t hls_media_segments_len(const HLSPlaylist *playlist) {
    return arrlen(playlist->media_segments);
}

size_t hls_get_variant_streams(struct HLSVariantStream **variant_streams,
                               const HLSPlaylist *playlist) {
    if (playlist->variant_streams == NULL) {
        return 0;
    }
    *variant_streams = playlist->variant_streams;
    return arrlen(playlist->variant_streams);
}

void hls_playlist_free(HLSPlaylist *playlist) {
    free(playlist->data);
    for (size_t i = 0; i < arrlen(playlist->lines); i++) {
        if (playlist->lines[i].is_tag) {
            arrfree(playlist->lines[i].tag->attributes);
            free(playlist->lines[i].tag);
        }
    }
    arrfree(playlist->lines);
    if (playlist->media_segments != NULL) {
        arrfree(playlist->media_segments);
    }
    if (playlist->variant_streams != NULL) {
        for (size_t i = 0; i < arrlen(playlist->variant_streams); i++) {
            arrfree(playlist->variant_streams[i].codecs);
        }
        arrfree(playlist->variant_streams);
    }
    free(playlist);
}
