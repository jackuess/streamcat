#define _GNU_SOURCE

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <mxml.h>

#include "http.h"
#include "output.h"
#include "vector2.h"

enum URL_TEMPLATE_IDENTIFIERS {
    _UNDEFINED,
    REPRESENTATION_ID,
    NUMBER,
    BANDWIDTH,
    TIME
};

struct URLTemplatePair {
    char *fmt_string;
    enum URL_TEMPLATE_IDENTIFIERS replacement_id;
};

typedef struct URLTemplatePair * URLTemplate;

URLTemplate parse_url_template(const char *str)
{
    bool tag_open = false;
    bool format_open = false;
    char *fmt = malloc(sizeof (char*) * strlen(str) + 1);
    size_t fmt_len = 0;
    enum URL_TEMPLATE_IDENTIFIERS replacement_tag = _UNDEFINED;
    URLTemplate template = vecnew(1, sizeof (template[0]));
    size_t n_pairs = 0;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '$') {
            if (tag_open) {
                if (replacement_tag == _UNDEFINED) {
                    fmt[fmt_len++] = '$';
                } else {
                    if (!format_open) {
                        fmt[fmt_len++] = '%';
                        if (replacement_tag == REPRESENTATION_ID) {
                            fmt[fmt_len++] = 's';
                        } else  {
                            fmt[fmt_len++] = 'l';
                            fmt[fmt_len++] = 'd';
                        }
                    }
                    fmt[fmt_len++] = '\0';

                    template = vecsetlen(template, n_pairs+1);
                    template[n_pairs].fmt_string = malloc(sizeof (char*) * fmt_len + 1);
                    strcpy(template[n_pairs].fmt_string, fmt);
                    template[n_pairs].replacement_id = replacement_tag;
                    n_pairs++;

                    replacement_tag = _UNDEFINED;
                    fmt_len = 0;
                }

                tag_open = false;
            } else {
                tag_open = true;
                format_open = false;
            }
        } else if (tag_open) {
            if (replacement_tag == _UNDEFINED) {
                switch (str[i]) {
                case 'R':
                    replacement_tag = REPRESENTATION_ID;
                    break;
                case 'N':
                    replacement_tag = NUMBER;
                    break;
                case 'B':
                    replacement_tag = BANDWIDTH;
                    break;
                case 'T':
                    replacement_tag = TIME;
                    break;
                }
            } else if (format_open) {
                fmt[fmt_len++] = str[i];
            } else if (str[i] == '%') {
				fmt[fmt_len++] = '%';
				format_open = true;
            }
        } else {
            fmt[fmt_len++] = str[i];
        }
    }
    if (fmt_len > 0) {
        fmt[fmt_len++] = '\0';
        template = vecsetlen(template, n_pairs+1);
        template[n_pairs].fmt_string = malloc(sizeof (char*) * fmt_len + 1);
        strcpy(template[n_pairs].fmt_string, fmt);
        template[n_pairs].replacement_id = _UNDEFINED;
    }

    free(fmt);

    return template;
}

char *url_template_format(const URLTemplate template, const char *representation_id, long number, long bandwidth, long time)
{
    char *result_parts[veclen(template)];
    size_t result_parts_len = 0;
    long *replacement = NULL;
    struct URLTemplatePair *pair;

    for (size_t i = 0; i < veclen(template); i++) {
        pair = &template[i];
        if (pair->replacement_id == _UNDEFINED) {
            size_t fmt_len = strlen(pair->fmt_string);
            result_parts_len += fmt_len;
            result_parts[i] = malloc(sizeof(char*) * fmt_len + 1);
            strcpy(result_parts[i], pair->fmt_string);
        } else {
            if (pair->replacement_id == REPRESENTATION_ID) {
                result_parts_len += asprintf(&result_parts[i], pair->fmt_string, representation_id);
            } else {
                switch (pair->replacement_id) {
                case REPRESENTATION_ID:
                    break;  // Handeled in above if
                case NUMBER:
                    replacement = &number;
                    break;
                case BANDWIDTH:
                    replacement = &bandwidth;
                    break;  // Handeled in above if
                case TIME:
                    replacement = &time;
                    break;
                case _UNDEFINED:
                    *replacement = 0;
                    break;
                }
                result_parts_len += asprintf(&result_parts[i], pair->fmt_string, *replacement);
            }
        }
    }

    char *result = malloc(result_parts_len * sizeof (char*) + 1);
    size_t result_len = 0;
    for (size_t i = 0; i < veclen(template); i++) {
        for (size_t j = 0; result_parts[i][j] != '\0'; j++) {
            result[result_len++] = result_parts[i][j];
        }
        free(result_parts[i]);
    }
    result[result_len] = '\0';

    return result;
}

void url_template_free(URLTemplate template)
{
    for (size_t j = 0; j < veclen(template); j++) {
        free(template[j].fmt_string);
    }
    vecfree(template);
}

struct MPD {
    struct AdaptationSet *adaptation_sets;
    mxml_node_t *_doc;
};

struct SegmentTime {
    long start;
    long part_duration;
    long part_count;
};

struct SegmentTemplate {
    const char *initialization;
    const char *media;
    struct SegmentTime *timeline;
};

struct Representation {
    long bandwidth;
    const char *id;
    const char *mime_type;
    const char *base_url;
    struct SegmentTemplate segment_template;
    // SegmentBase
    // SegmentList
};

struct AdaptationSet {
    const char *mime_type;
    struct SegmentTemplate segment_template;
    struct Representation *representations;
};

enum URL_TYPE {
    INITIALIZATION_URL,
    MEDIA_URL
};

struct SegmentTemplate get_segment_template(mxml_node_t *adaptation_set)
{
    struct SegmentTemplate template = {0};
    template.timeline = vecnew(3, sizeof (template.timeline[0]));

    mxml_node_t *root = mxmlFindElement(adaptation_set, adaptation_set, "SegmentTemplate", NULL, NULL, MXML_DESCEND_FIRST);
    template.initialization = mxmlElementGetAttr(root, "initialization");
    template.media = mxmlElementGetAttr(root, "media");

	long last_end = 0;
    size_t i = 0;
    mxml_node_t *timeline = mxmlFindElement(root, root, "SegmentTimeline", NULL, NULL, MXML_DESCEND_FIRST);
    for (
        mxml_node_t *node = mxmlFindElement(timeline, timeline, "S", NULL, NULL, MXML_DESCEND_FIRST);
        node != NULL;
        node = mxmlFindElement(node, timeline, "S", NULL, NULL, MXML_DESCEND_FIRST), i++
    ) {
        template.timeline = vecsetlen(template.timeline, i+1);
        struct SegmentTime *t = &template.timeline[i];
        const char *a;

        t->part_duration = strtol(mxmlElementGetAttr(node, "d"), NULL, 10);

        if ((a = mxmlElementGetAttr(node, "t")) != NULL) {
            t->start = strtol(a, NULL, 10);
        } else {
            t->start = last_end;
        }
        if ((a = mxmlElementGetAttr(node, "r")) != NULL) {
            t->part_count = strtol(a, NULL, 10) + 1;
        } else {
            t->part_count = 1;
        }

        last_end = t->start + t->part_duration * t->part_count;
    }

    return template;
}

struct MPD *mpd_parse(const char *buffer)
{
    struct MPD *mpd = calloc(1, sizeof (struct MPD));
    const char *TAG_ADAPTATION_SET = "AdaptationSet";
    const char *TAG_REPRESENTATION = "Representation";
    struct AdaptationSet *sets = vecnew(1, sizeof (struct AdaptationSet));
    size_t i = 0;

    mxml_node_t *root = mxmlLoadString(NULL, buffer, MXML_OPAQUE_CALLBACK);

    for (
        mxml_node_t *anode = mxmlFindElement(root, root, TAG_ADAPTATION_SET, NULL, NULL, MXML_DESCEND);
        anode != NULL;
        anode = mxmlFindElement(anode, root, TAG_ADAPTATION_SET, NULL, NULL, MXML_DESCEND)
    ) {
        sets = vecsetlen(sets, i + 1);
        sets[i].representations = vecnew(4, sizeof (sets[i].representations[0]));
        sets[i].mime_type = mxmlElementGetAttr(anode, "mimeType");
        sets[i].segment_template = get_segment_template(anode);

        size_t j = 0;
        for (
            mxml_node_t *rnode = mxmlFindElement(anode, anode, TAG_REPRESENTATION, NULL, NULL, MXML_DESCEND_FIRST);
            rnode != NULL;
            rnode = mxmlFindElement(rnode, anode, TAG_REPRESENTATION, NULL, NULL, MXML_DESCEND_FIRST)
        ) {
            sets[i].representations = vecsetlen(sets[i].representations, j+1);
            struct Representation *r = &sets[i].representations[j++];
            r->id = mxmlElementGetAttr(rnode, "id");
            r->bandwidth = strtol(mxmlElementGetAttr(rnode, "bandwidth"), NULL, 10);
            if ((r->mime_type = mxmlElementGetAttr(rnode, "mimeType")) == NULL) {
                r->mime_type = sets[i].mime_type;
            }
            mxml_node_t * t = mxmlFindElement(rnode, rnode, "SegmentTemplate", NULL, NULL, MXML_DESCEND_FIRST);
            if (t == NULL) {
                r->segment_template = sets[i].segment_template;  // TODO(Jacques): Copy
            } else {
                r->segment_template = get_segment_template(t);
            }
        }
        i++;
    }

    mpd->adaptation_sets = sets;
    mpd->_doc = root;
    return mpd;
}

void representation_free(struct Representation *repr)
{
    (void)repr;
    // vecfree(repr->segment_template.timeline);
}

void adaptation_set_free(struct AdaptationSet *set) {
    for (size_t i = 0; i < veclen(set->representations); i++) {
        representation_free(&set->representations[i]);
    }
    vecfree(set->segment_template.timeline);
    vecfree(set->representations);
}

void mpd_free(struct MPD *mpd)
{
    for (size_t i = 0; i < veclen(mpd->adaptation_sets); i++) {
        adaptation_set_free(&mpd->adaptation_sets[i]);
    }
    vecfree(mpd->adaptation_sets);
    mxmlDelete(mpd->_doc);
    free(mpd);
}

const struct Representation **mpd_get_representations(struct MPD *mpd)
{
    size_t len = 0;

    for (size_t i = 0; i < veclen(mpd->adaptation_sets); i++) {
        len += veclen(mpd->adaptation_sets[i].representations);
    }

    const struct Representation **reprs = calloc((len + 1), sizeof reprs);
    const struct Representation **r = reprs;
    for (size_t i = 0; i < veclen(mpd->adaptation_sets); i++) {
        struct AdaptationSet *set = &mpd->adaptation_sets[i];
        for (size_t j = 0; j < veclen(set->representations); j++) {
            *r = &set->representations[j];
            r++;
        }
    }
    r = NULL;

    return reprs;
}

long mpd_get_url(char **url, const char *base_url, const struct Representation *repr, enum URL_TYPE url_type, long time)
{
    long start_number = 0;  // TODO(Jacques): Replace zero with parsed startNumber
    size_t n = start_number;
    long next = 0;
    URLTemplate template = NULL;  // TODO(Jacques): Store template in Representation
    switch (url_type) {
        case INITIALIZATION_URL:
            template = parse_url_template(repr->segment_template.initialization);
            break;
        case MEDIA_URL:
            template = parse_url_template(repr->segment_template.media);
            break;
    }

    struct SegmentTime *timeline = repr->segment_template.timeline;
    for (size_t i = 0; i < veclen(timeline); i++) {
        struct SegmentTime *t = &timeline[i];

		if (time >= t->start && time < (t->start + (t->part_duration * t->part_count))) {
            size_t offset = (size_t)floor((float)(time - t->start) / (float)t->part_duration);
            long start = t->start + offset * t->part_duration;

            char *relative_url = url_template_format(template, repr->id, n, repr->bandwidth, start);
            *url = urljoin(base_url, relative_url);
            free(relative_url);

            next = start + t->part_duration;
            break;
        }
		n += t->part_count;
    }
    url_template_free(template);

    return next;
}

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
