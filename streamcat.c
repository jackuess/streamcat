#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "http.h"
#include "muxing.h"
#include "streamlisting.h"
#include "string.h"

static const char tmp_template[] = "/tmp/streamcatXXXXXX";
#define TMP_LEN sizeof(tmp_template) / sizeof(tmp_template[0])

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

FILE *create_temp_file(char **file_name) {
    *file_name = malloc(TMP_LEN);
    strncpy(*file_name, tmp_template, TMP_LEN);
    int fd = mkstemp(*file_name);
    if (fd < 0) {
        fprintf(stderr, "Could not create tempfile\n");
        return NULL;
    }
    close(fd);
    FILE *f = fopen(*file_name, "w+");
    return f;
}

static size_t
curl_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    return fwrite(buffer, size, nmemb, (FILE *)userp);
}

enum SCErrorCode concat_stream(FILE *f, char *stream_str) {
    enum SCErrorCode scerr = SC_SUCCESS;
    struct SCStream stream;
    char **urls = NULL;
    size_t num_urls = 0;
    CURL *curl_handle = NULL;

    if (stream_from_string(&stream, stream_str) == SC_SUCCESS) {
        urls = get_stream_urls(&stream, &num_urls, &scerr);
    } else {
        return SC_UNKNOW_FORMAT;
    }
    if (scerr != SC_SUCCESS) {
        return scerr;
    }
    if (urls == NULL) {
        return SC_UNKNOW_FORMAT;
    }

    if (f == NULL) {
        for (size_t i = 0; i < num_urls; i++) {
            printf("%s\n", urls[i]);
            free(urls[i]);
        }
    } else {
        curl_handle = curl_easy_init();
        CURLcode res;
        if (!curl_handle) {
            scerr = SC_UNKNOW_FORMAT;
            goto finally;
        }
        if (CURLE_OK != (res = curl_easy_setopt(curl_handle,
                                                CURLOPT_WRITEFUNCTION,
                                                curl_write_data))) {
            scerr = SC_UNKNOW_FORMAT;
            goto finally;
        }
        if (CURLE_OK !=
            (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)f))) {
            scerr = SC_UNKNOW_FORMAT;
            goto finally;
        }

        for (size_t i = 0; i < num_urls; i++) {
            if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, urls[i]))) {
                goto finally;
            }
            if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
                goto finally;
            }
            free(urls[i]);
        }
    }

finally:
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }
    free(urls);
    return scerr;
}

int main(int argc, char *argv[argc + 1]) {
    const char *output_file_name = NULL;
    int o;

    while ((o = getopt(argc, argv, "o:")) != -1) {
        switch (o) {
        case 'o':
            output_file_name = optarg;
            break;
        }
    };
    if (optind >= argc) {
        printf("No stream specified\n");
        return 1;
    }
    char **stream_ids = &argv[optind];

    enum SCErrorCode ret = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    if (str_starts_with(stream_ids[0], "http")) {
        ret = cat_streamlisting(stream_ids[0]);
    } else {
        int num_streams = argc - optind;
        if (output_file_name == NULL) {
            for (int i = 0; i < num_streams && ret == SC_SUCCESS; i++) {
                ret = concat_stream(NULL, stream_ids[i]);
            }
        } else {
            char **file_names = malloc(sizeof (file_names[0]) * num_streams);
            int num_file_names = 0;
            for (int i = 0; i < num_streams && ret == SC_SUCCESS; i++) {
                FILE *f = create_temp_file(&file_names[i]);
                if (f == NULL) {
                    goto cleanup_files;
                }
                num_file_names++;

                ret = concat_stream(f, stream_ids[i]);
                fclose(f);
                if (ret != SC_SUCCESS) {
                    ret = SC_UNKNOW_FORMAT;
                    goto cleanup_files;
                }
            }

            // TODO(Jacques): Avoid muxing if only one stream (HLS)
            mux(output_file_name, num_streams, file_names);

cleanup_files:
            for (int i = 0; i < num_file_names; i++) {
                unlink(file_names[i]);
                free(file_names[i]);
            }
            free(file_names);
        }
    }

    curl_global_cleanup();

    return ret;
}
