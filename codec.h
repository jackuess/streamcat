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

#ifndef codec_h_INCLUDED
#define codec_h_INCLUDED

enum SCCodecMediaType {
    SC_CODEC_AUDIO,
    SC_CODEC_VIDEO,
    SC_CODEC_UNKNOWN
};

struct SCCodec {
    const char *name;
    enum SCCodecMediaType codec_media_type;
};

struct SCCodec *parse_csv_codecs(char *data);

#endif // codec_h_INCLUDED

