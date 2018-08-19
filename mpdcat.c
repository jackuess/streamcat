#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "http.h"
#include "mpd.h"
#include "output.h"

int main(int argc, char *argv[argc + 1])
{
    if (argc != 2) {
        printf("No URL specified\n");
        return 1;
    }

    struct Response resp;
    const char *url = argv[1];
    struct MPD *mpd;
    const struct Representation **representations;

    curl_global_init(0);

    resp = http_get(url);
    dbg_print("mpdcat", "Code: %ld URL: %s\n", resp.code, resp.effective_url);
    dbg_print("mpdcat", "%s\n", resp.data);

    mpd = mpd_parse(resp.data);
    representations = mpd_get_representations(mpd);
    size_t num_representations = 0;
    for (const struct Representation *r; (r = representations[num_representations]) != NULL; num_representations++) {
        dbg_print("mpdcat", "%d\t%s\n", num_representations, r->mime_type);
    }

    char *output_url;
    mpd_get_url(&output_url, resp.effective_url, representations[0], INITIALIZATION_URL, 0);
    printf("%s\n", output_url);
    free(output_url);
    long start = 0;
    while ((start = mpd_get_url(&output_url, resp.effective_url, representations[0], MEDIA_URL, start))) {
        printf("%s\n", output_url);
        free(output_url);
    }

    free(representations);
    mpd_free(mpd);
    response_free(&resp);

    curl_global_cleanup();

    return resp.ok ? 0 : 2;
}
