#ifndef http_h_INCLUDED
#define http_h_INCLUDED

#include <stdbool.h>

#include <curl/curl.h>

struct Response {
    CURL *_curl;
    bool ok;
    char *effective_url;
    long code;
    char *data;
    size_t data_size;
};

struct Response http_get(const char *url);
void response_free(struct Response *resp);

char *urljoin(const char *base, const char *relative);

#endif // http_h_INCLUDED

