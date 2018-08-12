#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "output.h"

void response_init_data(struct Response *resp)
{
    resp->data = malloc(1);
    resp->data[0] = '\0';
    resp->data_size = 0;
}

static size_t response_append_data(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct Response *resp = (struct Response *)userp;

    resp->data = realloc(resp->data, resp->data_size + realsize + 1);
    memcpy(&(resp->data[resp->data_size]), data, realsize);
    resp->data_size += realsize;
    resp->data[resp->data_size] = '\0';

    return realsize;
}

struct Response http_get(const char *url)
{
    CURLcode res = CURLE_OK;
    struct Response resp = {0};
    resp._curl = curl_easy_init();

    if (!resp._curl) {
        dbg_print("mpdcat", "Unable to init Curl\n");
        goto finally;
    }

    if (CURLE_OK != (res = curl_easy_setopt(resp._curl, CURLOPT_FOLLOWLOCATION, true))) {
        dbg_print("mpdcat", "Unable to set FOLLOWLOCATION\n");
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(resp._curl, CURLOPT_URL, url))) {
        dbg_print("mpdcat", "Unable to set URL\n");
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(resp._curl, CURLOPT_WRITEFUNCTION, response_append_data))) {
        dbg_print("mpdcat", "Unable to set WRITEFUNCTION\n");
        goto finally;
    }
    response_init_data(&resp);
    if (CURLE_OK != (res = curl_easy_setopt(resp._curl, CURLOPT_WRITEDATA, &resp))) {
        dbg_print("mpdcat", "Unable to set WRITEDATA\n");
        goto finally;
    }

    if (CURLE_OK != (res = curl_easy_perform(resp._curl))) {
        goto finally;
    }

    curl_easy_getinfo(resp._curl, CURLINFO_RESPONSE_CODE, &resp.code);
    curl_easy_getinfo(resp._curl, CURLINFO_EFFECTIVE_URL, &resp.effective_url);

    finally:
        if (res != CURLE_OK) {
            dbg_print("libcurl", "%s\n", curl_easy_strerror(res));
        }

        resp.ok = (res == CURLE_OK);
        return resp;
}

void response_free(struct Response *resp)
{
    if (resp->_curl) {
         curl_easy_cleanup(resp->_curl);
    }
    if (resp->data) {
        free(resp->data);
    }
}

char *urljoin(const char *base, const char *relative)
{
    char *newurl;
    size_t base_len;
    size_t relative_start;
    size_t relative_len = strlen(relative);
    size_t lastchar;
    size_t i;

    for (base_len = strlen(base); base_len != 0; base_len--) {
        if (base[base_len - 1] == '/') {
            break;
        }
    }
    for (relative_start = 0; relative_start < relative_len; relative_start++) {
        if (relative[relative_start] != '/') {
            break;
        }
    }

    newurl = malloc(sizeof (newurl[0]) * (base_len + relative_len - relative_start) + 1);
    for (i = 0; i < base_len; i++) {
        newurl[i] = base[i];
    }
    lastchar = i;
    for (i = relative_start; i < relative_len; i++) {
        newurl[base_len + i - relative_start] = relative[i];
    }
    lastchar += i - relative_start;
    newurl[lastchar] = '\0';
    return newurl;
}
