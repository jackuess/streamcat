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

#include <ctype.h>
#include <stdbool.h>

#include "../vendor/arr/arr.h"

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
        for (; !eol && *c != '\0' && isspace(*c); c++) {}
        data = c;
    }

    return codecs;
}
