#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "http.h"
#include "muxing.h"
#include "mpd.h"
#include "vector2.h"

static const char tmp_template[] = "/tmp/mpdcatXXXXXX";
#define TMP_LEN sizeof (tmp_template)/sizeof (tmp_template[0])

enum MODE {
    LIST_REPRS,
    PRINT_REPR_URLS,
    DOWNLOAD_REPR_URLS
};

struct CMD {
    _Bool verbose;
    enum MODE mode;
    long *repr_index;
    const char *manifesturl;
    const char *output_file_name;
};

struct CMD parse_args(int argc, char *argv[argc+1]) {
    struct CMD cmd = {
        .mode = LIST_REPRS,
        .repr_index = vecnew(4, sizeof (long))
    };
    int o;
    size_t n_repr_index = 0;

    while ((o = getopt(argc, argv, "i:o:v")) != -1) {
        switch (o) {
        case 'i':
            cmd.repr_index = vecsetlen(cmd.repr_index, ++n_repr_index);
            cmd.repr_index[n_repr_index-1] = strtol(optarg, NULL, 10);
            cmd.mode = cmd.mode != DOWNLOAD_REPR_URLS ? PRINT_REPR_URLS : cmd.mode;
            break;
        case 'v':
            cmd.verbose = true;
            break;
        case 'o':
            cmd.output_file_name = optarg;
            cmd.mode = DOWNLOAD_REPR_URLS;
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

static size_t curl_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    return fwrite(buffer, size, nmemb, (FILE*)userp);
}

int concat_representations(FILE *f, const char *base_url, const struct Representation *repr) {
    CURL *curl_handle = curl_easy_init();
    CURLcode res = CURLE_OK;
    char *url;
    long start = 0;

    if (!curl_handle) {
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_write_data))) {
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)f))) {
        goto finally;
    }

    mpd_get_url(&url, base_url, repr, INITIALIZATION_URL, start);
    if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, url))) {
        goto finally;
    }
    if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
        goto finally;
    }
    free(url);
    while ((start = mpd_get_url(&url, base_url, repr, MEDIA_URL, start))) {
        if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, url))) {
            goto finally;
        }
        if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
            goto finally;
        }
        free(url);
	}

    finally:
        if (res != CURLE_OK) {
            fprintf(stderr, "(libcurl): %s\n", curl_easy_strerror(res));
        }
        if (curl_handle) {
            curl_easy_cleanup(curl_handle);
        }
        return res;
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
        for (size_t i = 0; i < veclen(cmd.repr_index); i++) {
            fprintrepr_urls(stdout, effective_url, representations[cmd.repr_index[i]]);
        }
    } else if (cmd.mode == DOWNLOAD_REPR_URLS) {
        unsigned int n_files = (unsigned int)veclen(cmd.repr_index);
        char *file_names[n_files];
        for (unsigned int i = 0; i < n_files; i++) {
            file_names[i] = malloc(TMP_LEN);
            strncpy(file_names[i], tmp_template, TMP_LEN);
            int fd = mkstemp(file_names[i]);
            if (fd < 0) {
                fprintf(stderr, "Could not create tempfile\n");
                success = false;
                goto finally;
            }
            close(fd);
            FILE *f = fopen(file_names[i], "w+");

            if (concat_representations(f, effective_url, representations[cmd.repr_index[i]]) != CURLE_OK) {
                fprintf(stderr, "Could not download index %ld\n", cmd.repr_index[i]);
                success = false;
                n_files = i;
                break;
            }
            fclose(f);
        }

        mux(cmd.output_file_name, n_files, file_names);

        for (size_t i = 0; i < n_files; i++) {
            unlink(file_names[i]);
            free(file_names[i]);
        }
    }

finally:
    vecfree(cmd.repr_index);
    free(effective_url);
    free(representations);
    mpd_free(mpd);

    curl_global_cleanup();

    return success ? 0 : 3;
}
