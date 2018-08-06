#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl.h>

#include "output.h"

struct StreamList {
    unsigned int n_streams;
    char *streams[1000];  // TODO(Jacques): Make dynamic
};

struct M3U8ParseContext {
    char *buffer;
    struct StreamList *streamlst;
};

void stream_list_print(const struct StreamList *lst)
{
    for (unsigned int i = 0; i < lst->n_streams; i++) {
        printf("%d: %s\n", i+1, lst->streams[i]);
    }
}

void stream_list_free(struct StreamList *lst)
{
    for (unsigned int i = 0; i < lst->n_streams; i++) {
        free(lst->streams[i]);
    }
    free(lst);
}

static size_t stream_list_parse_m3u8(char *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t i;
	size_t realsize = size * nmemb;
	size_t last_line_start = 0;
	struct M3U8ParseContext *parsectx = (struct M3U8ParseContext *)userp;
	struct StreamList *stream_list = parsectx->streamlst;

	for (i = 0; i < realsize; i++) {
    	if (buffer[i] == '\n') {
        	if (parsectx->buffer != NULL) {
                buffer[i] = '\0';
				stream_list->streams[stream_list->n_streams] = malloc(strlen(parsectx->buffer) + i - last_line_start + 1);
				strcpy(stream_list->streams[stream_list->n_streams], parsectx->buffer);
				strcat(stream_list->streams[stream_list->n_streams++], &buffer[last_line_start]);
				free(parsectx->buffer);
            	parsectx->buffer = NULL;
            } else if (buffer[last_line_start] != '#') {
                buffer[i] = '\0';
				stream_list->streams[stream_list->n_streams] = malloc(i - last_line_start + 1);
                strcpy(stream_list->streams[stream_list->n_streams++], &buffer[last_line_start]);
            }
            last_line_start = i + 1;
    	}
	}

	size_t chars_remaining = realsize - last_line_start;
	if (chars_remaining > 0 && buffer[last_line_start] != '#') {
    	if (parsectx->buffer == NULL) {
        	parsectx->buffer = malloc(chars_remaining + 1);
        	parsectx->buffer[0] = '\0';
    	} else {
        	parsectx->buffer = realloc(parsectx->buffer, strlen(parsectx->buffer) + chars_remaining + 1);
    	}
    	strncat(parsectx->buffer, &buffer[last_line_start], chars_remaining);
	}

	return realsize;
}

struct StreamList *get_stream_list(const char *url)
{
	CURL *curl_handle = curl_easy_init();
    CURLcode res = CURLE_OK;
    struct M3U8ParseContext *parsectx = malloc(sizeof (struct M3U8ParseContext));
    parsectx->streamlst = malloc(sizeof (struct StreamList));
    parsectx->streamlst->n_streams = 0;
    parsectx->buffer = NULL;
    struct StreamList *lst = parsectx->streamlst;

    if (!curl_handle) {
        goto error;
    }
	if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, url))) {
        goto error;
	}
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, stream_list_parse_m3u8))) {
        goto error;
    }

    if (CURLE_OK != (curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)parsectx))) {
        goto error;
    }

    dbg_print("piratepersist", "downloading %s\n", url);
    if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
		goto error;
    }
    curl_easy_cleanup(curl_handle);
    free(parsectx->buffer);
    free(parsectx);
    return lst;

    error:
        if (res != CURLE_OK)
            dbg_print("libcurl", "%s\n", curl_easy_strerror(res));
        if (curl_handle)
            curl_easy_cleanup(curl_handle);
        free(parsectx->buffer);
        stream_list_free(parsectx->streamlst);
        free(parsectx);
        return NULL;
}

int main(int argc, char *argv[argc+1])
{
    curl_global_init(0);
	struct StreamList *stream_list;
	struct StreamList *chunks;

    if (argc != 2) {
        return 1;
    }

    if ((stream_list = get_stream_list(argv[1]))) {
        // stream_list_print(stream_list);
        if ((chunks = get_stream_list(stream_list->streams[0]))) {
            stream_list_print(chunks);
            stream_list_free(chunks);
        }
        stream_list_free(stream_list);
    }
    curl_global_cleanup();

    return 0;
}
