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

