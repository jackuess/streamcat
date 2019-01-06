#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "http.h"
#include "streamlisting.h"
#include "string.h"

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
    fprintf(f, "%s|%s|%s\t%s\t%zu\n",
            protocol,
            url,
            stream->id,
            codecs,
            stream->bitrate);
    free(url);
    free(codecs);
}

enum SCErrorCode cat_streamlisting(char *url) {
    struct SCHTTPResponse resp = http_get(url);
    if (!resp.ok) {
        return SC_UNKNOW_FORMAT;
    }

    struct SCStreamList *streams = NULL;
    enum SCErrorCode scerr = sc_get_streams(&streams,
                                            resp.data,
                                            resp.data_size,
                                            resp.effective_url);
    if (scerr != SC_SUCCESS) {
        return scerr;
    }

    for (size_t i = 0; i < streams->len; i++) {
        fprint_stream(stdout, resp.effective_url, &streams->streams[i]);
    }

    sc_streams_free(streams);
    response_free(&resp);

    return SC_SUCCESS;
}

char **get_stream_urls(struct SCStream *stream, size_t *num_urls,
                       enum SCErrorCode *error) {
    *error = SC_SUCCESS;
    struct SCHTTPResponse resp = http_get(stream->url);
    if (!resp.ok) {
        *error = SC_UNKNOW_FORMAT;
    }
    stream->url = resp.effective_url;

    struct SCStreamSegmentData *segment_data;
    *error = sc_get_stream_segment_data(&segment_data,
                                        stream,
                                        resp.data,
                                        resp.data_size);

    if (*error != SC_SUCCESS) {
        return NULL;
    }

    fprintf(stderr, "Found %zu segments\n", segment_data->num_segments);

    struct SCStreamSegment segment;
    uint64_t time = 0;

    *error = sc_get_stream_segment(&segment, segment_data, SC_INITIALIZATION_URL, &time);
    if (*error != SC_SUCCESS) {
        response_free(&resp);
        return NULL;
    }

    *num_urls = 0;
    char **urls = NULL;
    if (segment.url == NULL) {
        urls = malloc(sizeof(urls[0]) * (segment_data->num_segments));
    } else {
        urls = malloc(sizeof(urls[0]) * (segment_data->num_segments + 1));
        urls[*num_urls] = malloc(strlen(segment.url) + 1);
        strcpy(urls[(*num_urls)++], segment.url);
    }
    sc_stream_segment_free(&segment, stream->protocol);

    while (SC_SUCCESS == (*error = sc_get_stream_segment(&segment, segment_data, SC_MEDIA_URL, &time))) {
        if (time == 0) {
            break;
        }
        urls[*num_urls] = malloc(strlen(segment.url) + 1);
        strcpy(urls[(*num_urls)++], segment.url);
        sc_stream_segment_free(&segment, stream->protocol);
    }
    sc_stream_segment_data_free(segment_data);
    response_free(&resp);

    return urls;
}

enum SCErrorCode stream_from_string(struct SCStream *stream, char *arg) {
    if (str_starts_with(arg, "HLS")) {
        stream->protocol = SC_PROTOCOL_HLS;
    } else if (str_starts_with(arg, "MPD")) {
        stream->protocol = SC_PROTOCOL_MPD;
    } else {
        return SC_UNKNOW_FORMAT;
    }

    const char delim = '|';

    char *url = arg;
    for (; *url != delim && *url != '\0'; url++) {}
    url++;

    char *id = url;
    for (; *id != delim && *id != '\0'; id++) {}
    *id = '\0';
    id++;

    stream->url = url;
    stream->id = id;

    return SC_SUCCESS;
}

enum SCErrorCode cat_streams(size_t num_streams, char **stream_str) {
    enum SCErrorCode scerr = SC_SUCCESS;
    for (size_t i = 0; i < num_streams; i++) {
        struct SCStream stream;
        char **urls = NULL;
        size_t num_urls = 0;
        if (stream_from_string(&stream, stream_str[i]) == SC_SUCCESS) {
            urls = get_stream_urls(&stream, &num_urls, &scerr);
        } else {
            return SC_UNKNOW_FORMAT;
        }
        if (scerr == SC_SUCCESS && urls != NULL) {
            for (size_t j = 0; j < num_urls; j++) {
                printf("%s\n", urls[j]);
                free(urls[j]);
            }
            free(urls);
        } else {
            break;
        }
    }
    return scerr;
}

int main(int argc, char *argv[argc + 1]) {
    if (argc < 2) {
        return 1;
    }

    enum SCErrorCode ret = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    if (str_starts_with(argv[1], "http")) {
        ret = cat_streamlisting(argv[1]);
    } else {
        ret = cat_streams(argc - 1, &argv[1]);
    }
    curl_global_cleanup();

    return ret;
}
