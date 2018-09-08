#define _GNU_SOURCE

#include <stdbool.h>

#include <mxml.h>

#include "http.h"
#include "mpd.h"
#include "vector.h"

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
    URLTemplate template = VECNEW(1, template[0]);

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

                    URLTemplate new_template = VECAPPEND(&template);
                    new_template->fmt_string = malloc(sizeof (char*) * fmt_len + 1);
                    strcpy(new_template->fmt_string, fmt);
                    new_template->replacement_id = replacement_tag;

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
        URLTemplate new_template = VECAPPEND(&template);
        new_template->fmt_string = malloc(sizeof (char*) * fmt_len + 1);
        strcpy(new_template->fmt_string, fmt);
        new_template->replacement_id = _UNDEFINED;
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
                    break;
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
    VECFREE(template);
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

struct AdaptationSet {
    const char *mime_type;
    struct SegmentTemplate segment_template;
    struct Representation *representations;
};

struct SegmentTemplate get_segment_template(mxml_node_t *adaptation_set)
{
    struct SegmentTemplate template = {0};
    const char *a;
    template.timeline = VECNEW(3, template.timeline[0]);

    mxml_node_t *root = mxmlFindElement(adaptation_set, adaptation_set, "SegmentTemplate", NULL, NULL, MXML_DESCEND_FIRST);
    template.initialization = mxmlElementGetAttr(root, "initialization");
    template.media = mxmlElementGetAttr(root, "media");

    if ((a = mxmlElementGetAttr(root, "startNumber")) != NULL) {
        template.start_number = strtol(a, NULL, 10);
    } else {
        template.start_number = 1;
    }

	long last_end = 0;
    mxml_node_t *timeline = mxmlFindElement(root, root, "SegmentTimeline", NULL, NULL, MXML_DESCEND_FIRST);
    for (
        mxml_node_t *node = mxmlFindElement(timeline, timeline, "S", NULL, NULL, MXML_DESCEND_FIRST);
        node != NULL;
        node = mxmlFindElement(node, timeline, "S", NULL, NULL, MXML_DESCEND_FIRST)
    ) {
        struct SegmentTime *t = VECAPPEND(&template.timeline);

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
    template.timeline_refs = 1;

    return template;
}

struct MPD *mpd_parse(const char *buffer)
{
    struct MPD *mpd = calloc(1, sizeof (struct MPD));
    const char *TAG_ADAPTATION_SET = "AdaptationSet";
    const char *TAG_REPRESENTATION = "Representation";
    struct AdaptationSet *sets = VECNEW(1, sets[0]);

    mxml_node_t *root = mxmlLoadString(NULL, buffer, MXML_OPAQUE_CALLBACK);

    for (
        mxml_node_t *anode = mxmlFindElement(root, root, TAG_ADAPTATION_SET, NULL, NULL, MXML_DESCEND);
        anode != NULL;
        anode = mxmlFindElement(anode, root, TAG_ADAPTATION_SET, NULL, NULL, MXML_NO_DESCEND)
    ) {
        struct AdaptationSet *new_set = VECAPPEND(&sets);
        new_set->representations = VECNEW(4, new_set->representations[0]);
        new_set->mime_type = mxmlElementGetAttr(anode, "mimeType");
        new_set->segment_template = get_segment_template(anode);

        for (
            mxml_node_t *rnode = mxmlFindElement(anode, anode, TAG_REPRESENTATION, NULL, NULL, MXML_DESCEND_FIRST);
            rnode != NULL;
            rnode = mxmlFindElement(rnode, anode, TAG_REPRESENTATION, NULL, NULL, MXML_NO_DESCEND)
        ) {
            struct Representation *r = VECAPPEND(&new_set->representations);
            r->id = mxmlElementGetAttr(rnode, "id");
            r->bandwidth = strtol(mxmlElementGetAttr(rnode, "bandwidth"), NULL, 10);
            if ((r->mime_type = mxmlElementGetAttr(rnode, "mimeType")) == NULL) {
                r->mime_type = new_set->mime_type;
            }
            mxml_node_t * t = mxmlFindElement(rnode, rnode, "SegmentTemplate", NULL, NULL, MXML_DESCEND_FIRST);
            if (t == NULL) {
                r->segment_template = new_set->segment_template;
                new_set->segment_template.timeline_refs++;
            } else {
                r->segment_template = get_segment_template(t);
            }
        }
    }

    mpd->adaptation_sets = sets;
    mpd->_doc = root;
    return mpd;
}

void representation_free(struct Representation *repr)
{
    if (--(repr->segment_template.timeline_refs) == 0) {
        VECFREE(repr->segment_template.timeline);
    }
}

void adaptation_set_free(struct AdaptationSet *set) {
    for (size_t i = 0; i < veclen(set->representations); i++) {
        representation_free(&set->representations[i]);
    }
    if (--(set->segment_template.timeline_refs) == 0) {
        VECFREE(set->segment_template.timeline);
    }
    VECFREE(set->representations);
}

void mpd_free(struct MPD *mpd)
{
    for (size_t i = 0; i < veclen(mpd->adaptation_sets); i++) {
        adaptation_set_free(&mpd->adaptation_sets[i]);
    }
    VECFREE(mpd->adaptation_sets);
    mxmlDelete(mpd->_doc);
    free(mpd);
}

size_t mpd_get_representations(struct Representation **ret, const struct MPD *mpd) {
    size_t len = 0;

    for (size_t i = 0; i < veclen(mpd->adaptation_sets); i++) {
        len += veclen(mpd->adaptation_sets[i].representations);
    }

    struct Representation *repr = calloc(len, sizeof (repr[0]));
    *ret = repr;
    for (size_t i = 0; i < veclen(mpd->adaptation_sets); i++) {
        struct AdaptationSet *set = &mpd->adaptation_sets[i];
        for (size_t j = 0; j < veclen(set->representations); j++) {
            *repr = set->representations[j];
            repr++;
        }
    }

    return len;
}

size_t mpd_get_url_count(const struct Representation *repr) {
    size_t count = 0;

    for (size_t i = 0; i < veclen(repr->segment_template.timeline); i++) {
        count += repr->segment_template.timeline[i].part_count;
    }

    return count;
}

long mpd_get_url(char **url, const char *base_url, const struct Representation *repr, enum URL_TYPE url_type, long time)
{
    long start_number = repr->segment_template.start_number;
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
            size_t offset = (time - t->start) / t->part_duration;
            long start = t->start + offset * t->part_duration;

            char *relative_url = url_template_format(template, repr->id, n+offset, repr->bandwidth, start);
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
