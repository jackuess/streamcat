#include <stdio.h>

#include "http.h"
#include "streamlisting.h"

const char *protocol_to_string(enum SCStreamProtocol protocol) {
    static const char *protocol_hls = "HLS";
    static const char *protocol_mpd = "MPD";
    switch (protocol) {
        case SC_PROTOCOL_HLS:
            return protocol_hls;
        case SC_PROTOCOL_MPD:
            return protocol_mpd;
    }
    return NULL;
}

void fprint_stream(FILE *f, const char *base_url,
                   const struct SCStream *stream) {
    const char *protocol = protocol_to_string(stream->protocol);
    fprintf(f, "%s|%s|%s\n",
            protocol,
            urljoin(base_url, stream->url),
            stream->id);
}

int main(int argc, char *argv[argc + 1]) {
    if (argc != 2) {
        return 1;
    }

    struct SCHTTPResponse resp = http_get(argv[1]);
    if (!resp.ok) {
        return 2;
    }

    struct SCStreamList *streams = NULL;
    enum SCErrorCode scerr = sc_get_streams(&streams, resp.data, resp.data_size);
    if (scerr != SC_SUCCESS) {
        return 3;
    }

    for (size_t i = 0; i < streams->len; i++) {
        fprint_stream(stdout, resp.effective_url, &streams->streams[i]);
    }

    return 0;
}
