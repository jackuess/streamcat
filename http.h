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
