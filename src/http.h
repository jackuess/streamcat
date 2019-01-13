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

#ifndef http_h_INCLUDED
#define http_h_INCLUDED

#include <stdbool.h>
#include <stddef.h>

struct SCHTTPResponse {
    bool ok;
    char *effective_url;
    long code;
    char *data;
    size_t data_size;
    void *userp;
};

struct SCHTTPResponse http_get(const char *url);
void response_free(struct SCHTTPResponse *resp);

char *urljoin(const char *base, const char *relative);

#endif  // http_h_INCLUDED
