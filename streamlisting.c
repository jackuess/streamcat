#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl.h>

#include "output.h"
#include "streamlisting.h"

struct M3U8ParseContext {
    char *buffer;
    struct StreamList *streamlst;
};

void stream_list_print(FILE *f, const struct StreamList *lst)
{
    for (unsigned int i = 0; i < lst->n_streams; i++) {
        fprintf(f, "%d\t%s\n", i, lst->streams[i]);
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

struct StreamList *m3u8_get_stream_list(const char *url)
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

    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)parsectx))) {
        goto error;
    }

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

static size_t curl_write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    return fwrite(buffer, size, nmemb, (FILE *)userp);
}

int concat_streams(FILE *stream, const struct StreamList *lst)
{
    CURL *curl_handle = curl_easy_init();
    CURLcode res = CURLE_OK;
    const char *url;

    if (!curl_handle) {
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_write_data))) {
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)stream))) {
        goto finally;
    }

	for (unsigned int i = 0; i < lst->n_streams; i++) {
    	url = lst->streams[i];
        dbg_print("piratepersist", "downloading %s\n", url);

        if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, url))) {
            goto finally;
        }
        if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
            goto finally;
        }
	}

    finally:
        if (res != CURLE_OK) {
            dbg_print("libcurl", "%s\n", curl_easy_strerror(res));
        }
        if (curl_handle) {
            curl_easy_cleanup(curl_handle);
        }
        return res;
}
