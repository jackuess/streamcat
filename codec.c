#include <stdbool.h>

#include "vendor/arr/arr.h"

#include "codec.h"
#include "string.h"

struct SCCodec *parse_csv_codecs(char *data) {
    struct SCCodec *codecs = arrnew(0, sizeof codecs[0]);

    bool eol = false;
    char *c = data;
    while (!eol) {
        struct SCCodec *codec = ARRAPPEND(&codecs);
        for (; *c != ',' && *c != '\0'; c++) {}
        if (*c == '\0') {
            eol = true;
        } else {
            *c = '\0';
        }

        codec->name = data;
        codec->codec_media_type = SC_CODEC_UNKNOWN;
        if (str_starts_with(codec->name, "mp4a")) {
            codec->codec_media_type = SC_CODEC_AUDIO;
        } else if (str_starts_with(codec->name, "avc1")) {
            codec->codec_media_type = SC_CODEC_VIDEO;
        }

        c++;
        data = c;
    }

    return codecs;
}
