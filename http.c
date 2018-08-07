#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "curl.h"

#include "http.h"
#include "output.h"

void string_init(struct String *str)
{
    str->data = malloc(1);
    str->data[0] = '\0';
    str->size = 0;
}

static size_t append_to_string(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct String *buffer = (struct String *)userp;

    buffer->data = realloc(buffer->data, buffer->size + realsize + 1);
    memcpy(&(buffer->data[buffer->size]), data, realsize);
    buffer->size += realsize;
    buffer->data[buffer->size] = '\0';

    return realsize;
}

int http_get_as_string(struct String *dest, const char *url)
{
    CURL *curl = curl_easy_init();
    CURLcode res = CURLE_OK;

    if (!curl) {
        dbg_print("mpdcat", "Unable to init Curl\n");
        goto finally;
    }

    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true))) {
        dbg_print("mpdcat", "Unable to set FOLLOWLOCATION\n");
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_URL, url))) {
        dbg_print("mpdcat", "Unable to set URL\n");
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_to_string))) {
        dbg_print("mpdcat", "Unable to set WRITEFUNCTION\n");
        goto finally;
    }
    string_init(dest);
    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, dest))) {
        dbg_print("mpdcat", "Unable to set WRITEDATA\n");
        goto finally;
    }

    if (CURLE_OK != (res = curl_easy_perform(curl))) {
        goto finally;
    }

    finally:
        if (res != CURLE_OK) {
            dbg_print("libcurl", "%s\n", curl_easy_strerror(res));
        }
        if (curl) {
            curl_easy_cleanup(curl);
        }

        return res == CURLE_OK;
}
