#define _POSIX_C_SOURCE 200809L

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "http.h"
#include "muxing.h"
#include "mpd.h"
#include "vector2.h"

#define ANSI_BG_GREEN "\033[42m"
#define ANSI_BG_BLUE "\033[44m"
#define ANSI_FG_BLACK "\033[30m"
#define ANSI_RESET "\033[0m\n"

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
        .repr_index = VECNEW(4, long)
    };
    int o;

    while ((o = getopt(argc, argv, "i:o:v")) != -1) {
        switch (o) {
        case 'i':
            cmd.mode = cmd.mode != DOWNLOAD_REPR_URLS ? PRINT_REPR_URLS : cmd.mode;
            long *repr_index = VECAPPEND(&cmd.repr_index);
            *repr_index = strtol(optarg, NULL, 10);
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

int concat_representations(FILE *f, const char *base_url, const struct Representation *repr,
                           void(*on_part_downloaded)(size_t n_parts_total, size_t n_parts, void* userp),
                           void *userp) {
    CURL *curl_handle = curl_easy_init();
    CURLcode res = CURLE_OK;
    char *url;
    long start = 0;
    size_t n_parts_downloaded = 0;

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
    size_t n_parts_total = mpd_get_url_count(repr);
    while ((start = mpd_get_url(&url, base_url, repr, MEDIA_URL, start))) {
        if (CURLE_OK != (res = curl_easy_setopt(curl_handle, CURLOPT_URL, url))) {
            goto finally;
        }
        if (CURLE_OK != (res = curl_easy_perform(curl_handle))) {
            goto finally;
        }
        if (on_part_downloaded) {
            (*on_part_downloaded)(n_parts_total, ++n_parts_downloaded, userp);
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

int cmp_repr(const void *first, const void *second) {
    const struct Representation *rfirst = first;
    const struct Representation *rsecond = second;

    int mimetype_diff = strcmp(rfirst->mime_type, rsecond->mime_type);
    if (mimetype_diff != 0) {
        return mimetype_diff;
    }

    long bandwidth_diff = rfirst->bandwidth - rsecond->bandwidth;
    if (bandwidth_diff < 0) {
        return -1;
    } else if (bandwidth_diff > 0) {
        return 1;
    } else {
        return 0;
    }

}

struct ProgressMeta {
    const char *ansi_color_fg;
    const char *ansi_color_bg;
    char *progress_bar;
};

void print_progress(size_t n_parts_total, size_t n_parts, void *userp) {
    struct ProgressMeta *meta = userp;
    size_t bar_size = strlen(meta->progress_bar);
    size_t curr_pos = n_parts * bar_size / n_parts_total;
    size_t prev_pos = (n_parts - 1) * bar_size / n_parts_total;

    if (curr_pos > prev_pos) {
        fprintf(stderr, "%s%s%c", meta->ansi_color_fg, meta->ansi_color_bg, meta->progress_bar[curr_pos - 1]);
    }
    if (n_parts == n_parts_total) {
        fprintf(stderr, ANSI_RESET);
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

    struct Representation *representations = NULL;
    size_t num_representations = mpd_get_representations(&representations, mpd);

    qsort(representations, num_representations, sizeof (representations[0]), &cmp_repr);

    for (size_t i = 0; i < num_representations; i++) {
        if (cmd.verbose) {
            fprintrepr(stderr, i, &representations[i]);
        } else if (cmd.mode == LIST_REPRS) {
            fprintrepr(stdout, i, &representations[i]);
        }
    }

    if (cmd.mode == PRINT_REPR_URLS) {
        for (size_t i = 0; i < veclen(cmd.repr_index); i++) {
            fprintrepr_urls(stdout, effective_url, &representations[cmd.repr_index[i]]);
        }
    } else if (cmd.mode == DOWNLOAD_REPR_URLS) {
        unsigned int n_files = (unsigned int)veclen(cmd.repr_index);
        char *file_names[n_files];
        char bandwidthstr[14];
        struct winsize ws;
        ioctl(STDERR_FILENO, TIOCGWINSZ, &ws);
        struct ProgressMeta meta;
        meta.ansi_color_fg = ANSI_FG_BLACK;
        meta.progress_bar = malloc(ws.ws_col + 1);
        meta.progress_bar[ws.ws_col] = '\0';

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

            struct Representation *repr = &representations[cmd.repr_index[i]];

            if (strcmp(repr->mime_type, "audio/mp4") == 0) {
                meta.ansi_color_bg = ANSI_BG_BLUE;
            } else {
                meta.ansi_color_bg = ANSI_BG_GREEN;
            }
            formatbandwidth(bandwidthstr, repr->bandwidth);
            snprintf(meta.progress_bar, ws.ws_col + 1, "%s %s downloaded to %s", repr->mime_type, bandwidthstr, file_names[i]);
            for (size_t j = strlen(meta.progress_bar); j < ws.ws_col; j++) {
                meta.progress_bar[j] = ' ';
            }

            if (concat_representations(f, effective_url, repr, &print_progress, &meta) != CURLE_OK) {
                fprintf(stderr, "Could not download index %ld\n", cmd.repr_index[i]);
                success = false;
                n_files = i;
                break;
            }
            fclose(f);
        }
        free(meta.progress_bar);

        mux(cmd.output_file_name, n_files, file_names);

        for (size_t i = 0; i < n_files; i++) {
            unlink(file_names[i]);
            free(file_names[i]);
        }
    }

finally:
    VECFREE(cmd.repr_index);
    free(effective_url);
    free(representations);
    mpd_free(mpd);

    curl_global_cleanup();

    return success ? 0 : 3;
}
