#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl.h>

#include "output.h"

struct StreamList {
    unsigned int n_streams;
    char *_buffer;
    char *streams[1000];  // TODO(Jacques): Make dynamic
};

struct M3U8ParseContext {
    char *buffer;
    

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
    free(lst->_buffer);
    free(lst);
}

static size_t stream_list_parse_m3u8(char *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t i;
	size_t realsize = size * nmemb;
	size_t last_line_start = 0;
	struct StreamList *stream_list = (struct StreamList *)userp;

	for (i = 0; i < realsize; i++) {
    	if (buffer[i] == '\n') {
        	if (stream_list->_buffer != NULL) {
                buffer[i] = 0;
				stream_list->streams[stream_list->n_streams] = malloc(strlen(stream_list->_buffer) + strlen(&buffer[last_line_start] + 1));
				strcpy(stream_list->streams[stream_list->n_streams], stream_list->_buffer);
				strcat(stream_list->streams[stream_list->n_streams++], &buffer[last_line_start]);
				free(stream_list->_buffer);
            	stream_list->_buffer = NULL;
            } else if (buffer[last_line_start] != '#') {
                buffer[i] = 0;
				stream_list->streams[stream_list->n_streams] = malloc(strlen(&buffer[last_line_start])+1);
                strcpy(stream_list->streams[stream_list->n_streams++], &buffer[last_line_start]);
            }
            last_line_start = i + 1;
    	}
	}
	if (last_line_start < realsize && buffer[last_line_start] != '#') {
    	// TODO(Jacques): Check if _buffer is not NULL
    	stream_list->_buffer = malloc(realsize - last_line_start + 1);
    	strncpy(stream_list->_buffer, &buffer[last_line_start], realsize - last_line_start);
    	stream_list->_buffer[realsize - last_line_start] = 0;
	}

	return realsize;
}

struct StreamList *get_stream_list(const char *url)
{
	CURL *curl_handle = curl_easy_init();
    CURLcode res = CURLE_OK;
    struct StreamList *stream_list = malloc(sizeof (struct StreamList));
    stream_list->n_streams = 0;
    stream_list->_buffer = NULL;

    if (!curl_handle) {
        goto error;
    }
	if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, url))) {
        goto error;
	}
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, stream_list_parse_m3u8))) {
        goto error;
    }

    if (CURLE_OK != (curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)stream_list))) {
        goto error;
    }

    dbg_print("piratepersist", "downloading %s\n", url);
    if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
		goto error;
    }
    curl_easy_cleanup(curl_handle);
    return stream_list;

    error:
        if (res != CURLE_OK)
            dbg_print("libcurl", "%s\n", curl_easy_strerror(res));
        if (curl_handle)
            curl_easy_cleanup(curl_handle);
        stream_list_free(stream_list);
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
