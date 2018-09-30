#include <stdbool.h>

#include "vendor/arr/arr.h"

#include "codec.h"
#include "string.h"

struct Codec *parse_csv_codecs(char *data) {
    struct Codec *codecs = arrnew(0, sizeof codecs[0]);

    bool eol = false;
    char *c = data;
    while (!eol) {
        struct Codec *codec = ARRAPPEND(&codecs);
        for (; *c != ',' && *c != '\0'; c++) {}
        if (*c == '\0') {
            eol = true;
        } else {
            *c = '\0';
        }

        codec->name = data;
        codec->codec_media_type = CODEC_UNKNOWN;
        if (str_starts_with(codec->name, "mp4a")) {
            codec->codec_media_type = CODEC_AUDIO;
        } else if (str_starts_with(codec->name, "avc1")) {
            codec->codec_media_type = CODEC_VIDEO;
        }

        c++;
        data = c;
    }

    return codecs;
}
