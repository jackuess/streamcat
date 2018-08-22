#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "http.h"
#include "mpd.h"

enum MODE {
    LIST_REPRS,
    PRINT_REPR_URLS
};

struct CMD {
    _Bool verbose;
    enum MODE mode;
    long repr_index;
    const char *manifesturl;
};

struct CMD parse_args(int argc, char *argv[argc+1]) {
    struct CMD cmd = {
        .mode = LIST_REPRS
    };
    int o;

    while ((o = getopt(argc, argv, "i:v")) != -1) {
        switch (o) {
        case 'i':
            cmd.repr_index = strtol(optarg, NULL, 10);
            cmd.mode = PRINT_REPR_URLS;
            break;
        case 'v':
            cmd.verbose = true;
            break;
        }
	};
	if (optind >= argc) {
        printf("No URL specified\n");
        exit(1);
    }
    cmd.manifesturl = argv[optind];
    return cmd;
}

_Bool get_mpd(struct MPD **mpd, char **effective_url, const struct CMD *cmd) {
    struct Response resp = http_get(cmd->manifesturl);
    if (!resp.ok) {
        response_free(&resp);
        return false;
    };
    if (cmd->verbose) {
        fprintf(stderr, "Downloaded manifest from: %s\n", resp.effective_url);
        fprintf(stderr, "MANIFEST:%s\n", resp.data);
    }
    size_t urllen = strlen(resp.effective_url) + 1;
    *effective_url = malloc(urllen);
    strcpy(*effective_url, resp.effective_url);
    effective_url[urllen] = '\0';

    *mpd = mpd_parse(resp.data);
    response_free(&resp);

    return mpd != NULL;
}

void formatbandwidth(char *str, long bandwidth) {
    const char *symbol;
    const char tbps[] = "Tbit/s";
    const char gbps[] = "Gbit/s";
    const char mbps[] = "Mbit/s";
    const char kbps[] = "kbit/s";
    const char bps[] = "bit/s";
    double denominator;

    if (bandwidth >= 1000000000000) {
        symbol = tbps;
        denominator = 1000000000000;
    } else if (bandwidth >= 1000000000) {
        symbol = gbps;
        denominator = 1000000000;
    } else if (bandwidth >= 1000000) {
        symbol = mbps;
        denominator = 1000000;
    } else if (bandwidth >= 1000) {
        symbol = kbps;
        denominator = 1000;
    } else {
        symbol = bps;
        denominator = 1;
    }
    snprintf(str, 14, "%.2f %s", (double)bandwidth / denominator, symbol);
}

void fprintrepr(FILE *f, size_t index, const struct Representation *repr) {
    char bandwidthstr[14];
    formatbandwidth(bandwidthstr, repr->bandwidth);
    fprintf(f, "%ld\tmimetype: %s\tbandwidth: %s\n", index, repr->mime_type, bandwidthstr);
}

void fprintrepr_urls(FILE *f, const char *base_url, const struct Representation *repr) {
    char *output_url;
    long start = 0;

    mpd_get_url(&output_url, base_url, repr, INITIALIZATION_URL, start);
    fprintf(f, "%s\n", output_url);
    free(output_url);

    while ((start = mpd_get_url(&output_url, base_url, repr, MEDIA_URL, start))) {
        fprintf(f, "%s\n", output_url);
        free(output_url);
    }
}

int main(int argc, char *argv[argc + 1])
{
    struct CMD cmd = parse_args(argc, argv);
    _Bool success = false;

    curl_global_init(0);

    struct MPD *mpd = NULL;
    char *effective_url = NULL;
    success = get_mpd(&mpd, &effective_url, &cmd);
    if (!success) {
        free(effective_url);
        return 2;
    }

    const struct Representation **representations = mpd_get_representations(mpd);
    size_t num_representations;
    for (num_representations = 0; representations[num_representations] != NULL; num_representations++) {
        if (cmd.verbose) {
            fprintrepr(stderr, num_representations, representations[num_representations]);
        } else if (cmd.mode == LIST_REPRS) {
            fprintrepr(stdout, num_representations, representations[num_representations]);
        }
    }

    if (cmd.mode == PRINT_REPR_URLS) {
        fprintrepr_urls(stdout, effective_url, representations[cmd.repr_index]);
    }

    free(effective_url);
    free(representations);
    mpd_free(mpd);

    curl_global_cleanup();

    return success ? 0 : 3;
}
