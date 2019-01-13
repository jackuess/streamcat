/*
 * Copyright 2019 Jacques de Laval

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "http.h"
#include "output.h"
#include "string.h"

static void response_init_data(struct SCHTTPResponse *resp) {
    resp->data = malloc(1);
    resp->data[0] = '\0';
    resp->data_size = 0;
}

static size_t
response_append_data(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct SCHTTPResponse *resp = (struct SCHTTPResponse *)userp;

    resp->data = realloc(resp->data, resp->data_size + realsize + 1);
    memcpy(&(resp->data[resp->data_size]), data, realsize);
    resp->data_size += realsize;
    resp->data[resp->data_size] = '\0';

    return realsize;
}

struct SCHTTPResponse http_get(const char *url) {
    CURLcode res = CURLE_OK;
    struct SCHTTPResponse resp = {0};
    resp.userp = curl_easy_init();

    if (!resp.userp) {
        dbg_print("mpdcat", "Unable to init Curl\n");
        goto finally;
    }

    if (CURLE_OK !=
        (res = curl_easy_setopt(resp.userp, CURLOPT_FOLLOWLOCATION, true))) {
        dbg_print("mpdcat", "Unable to set FOLLOWLOCATION\n");
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(resp.userp, CURLOPT_URL, url))) {
        dbg_print("mpdcat", "Unable to set URL\n");
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(resp.userp,
                                            CURLOPT_WRITEFUNCTION,
                                            response_append_data))) {
        dbg_print("mpdcat", "Unable to set WRITEFUNCTION\n");
        goto finally;
    }
    response_init_data(&resp);
    if (CURLE_OK !=
        (res = curl_easy_setopt(resp.userp, CURLOPT_WRITEDATA, &resp))) {
        dbg_print("mpdcat", "Unable to set WRITEDATA\n");
        goto finally;
    }

    if (CURLE_OK != (res = curl_easy_perform(resp.userp))) {
        goto finally;
    }

    curl_easy_getinfo(resp.userp, CURLINFO_RESPONSE_CODE, &resp.code);
    curl_easy_getinfo(resp.userp, CURLINFO_EFFECTIVE_URL, &resp.effective_url);

finally:
    if (res != CURLE_OK) {
        dbg_print("libcurl", "%s\n", curl_easy_strerror(res));
    }

    resp.ok = (res == CURLE_OK);
    return resp;
}

void response_free(struct SCHTTPResponse *resp) {
    if (resp->userp != NULL) {
        curl_easy_cleanup(resp->userp);
    }
    if (resp->data) {
        free(resp->data);
    }
}

char *urljoin(const char *base, const char *relative) {
    char *newurl;
    size_t base_len;
    size_t relative_start;
    size_t relative_len = strlen(relative);
    size_t lastchar;
    size_t i;

    if (str_starts_with(relative, "http://") ||
          str_starts_with(relative, "https://")) {
        newurl = malloc(strlen(relative) + 1);
        newurl = strcpy(newurl, relative);
        return newurl;
    }

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

    newurl = malloc(
        sizeof(newurl[0]) * (base_len + relative_len - relative_start) + 1);
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
