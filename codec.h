#ifndef codec_h_INCLUDED
#define codec_h_INCLUDED

enum CodecMediaType {
    CODEC_AUDIO,
    CODEC_VIDEO,
    CODEC_UNKNOWN
};

struct Codec {
    const char *name;
    enum CodecMediaType codec_media_type;
};

struct Codec *parse_csv_codecs(char *data);

#endif // codec_h_INCLUDED

