#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

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

char *codecs_to_string(const struct SCCodec *codecs, size_t num_codecs) {
    char *ret = malloc(1);
    ret[0] = '\0';
    size_t ret_len = 0;
    const char delim = ',';

    for (size_t i = 0; i < num_codecs; i++) {
        ret_len += strlen(codecs[i].name) + 1;
        ret = realloc(ret, ret_len + 1);
        ret = strcat(ret, codecs[i].name);
        ret = strncat(ret, &delim, 1);
    }
    if (ret_len > 0) {
        ret[ret_len-1] = '\0';
    }

    return ret;
}

void fprint_stream(FILE *f, const char *base_url,
                   const struct SCStream *stream) {
    const char *protocol = protocol_to_string(stream->protocol);
    char *url = urljoin(base_url, stream->url);
    char *codecs = codecs_to_string(stream->codecs, stream->num_codecs);
    fprintf(f, "%s\t%s\t%s\t%s\t%zu\n",
            protocol,
            url,
            stream->id,
            codecs,
            stream->bitrate);
    free(url);
    free(codecs);
}

int main(int argc, char *argv[argc + 1]) {
    if (argc != 2) {
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    struct SCHTTPResponse resp = http_get(argv[1]);
    if (!resp.ok) {
        return 2;
    }

    struct SCStreamList *streams = NULL;
    enum SCErrorCode scerr = sc_get_streams(&streams,
                                            resp.data,
                                            resp.data_size,
                                            resp.effective_url);
    if (scerr != SC_SUCCESS) {
        return 3;
    }

    for (size_t i = 0; i < streams->len; i++) {
        fprint_stream(stdout, resp.effective_url, &streams->streams[i]);
    }

    sc_streams_free(streams);
    response_free(&resp);
    curl_global_cleanup();

    return 0;
}
