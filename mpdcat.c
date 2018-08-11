#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <curl.h>
#include <mxml.h>

#include "http.h"
#include "output.h"
#include "vector.h"

enum URL_TEMPLATE_IDENTIFIERS {
    _UNDEFINED,
    REPRESENTATION_ID,
    NUMBER,
    BANDWIDTH,
    TIME
};

#define MAX_REPLACEMENT_IDS 128

struct URLTemplate {
    char *fmt_strings[MAX_REPLACEMENT_IDS];
    size_t num_fmt_strings;
    enum URL_TEMPLATE_IDENTIFIERS replacement_ids[MAX_REPLACEMENT_IDS];
};

struct URLTemplate parse_url_template(const char *str)
{
    bool tag_open = false;
    bool format_open = false;
    char *fmt = malloc(sizeof (char*) * strlen(str) + 1);
    size_t fmt_len = 0;
    enum URL_TEMPLATE_IDENTIFIERS replacement_tag = _UNDEFINED;
    size_t num_replacement_ids = 0;
    struct URLTemplate template = {0};
    template.replacement_ids[0] = _UNDEFINED;
    template.num_fmt_strings = 0;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '$') {
            if (tag_open) {
                if (replacement_tag == _UNDEFINED) {
                    fmt[fmt_len++] = '$';
                } else {
                    if (!format_open) {
                        fmt[fmt_len++] = '%';
                        fmt[fmt_len++] = 'd';
                    }
                    fmt[fmt_len++] = '\0';

                    template.fmt_strings[template.num_fmt_strings] = malloc(sizeof (char*) * strlen(fmt) + 1);
                    strcpy(template.fmt_strings[template.num_fmt_strings++], fmt);

                    template.replacement_ids[num_replacement_ids++] = replacement_tag;
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
    template.replacement_ids[num_replacement_ids] = _UNDEFINED;
    if (fmt_len > 0) {
        fmt[fmt_len++] = '\0';
        template.fmt_strings[template.num_fmt_strings] = malloc(sizeof (char*) * strlen(fmt) + 1);
        strcpy(template.fmt_strings[template.num_fmt_strings++], fmt);
    }


    return template;
}

char *url_template_format(const struct URLTemplate template, int representation_id, int number, int bandwidth, int time)
{
    char *result_parts[template.num_fmt_strings];
    size_t result_parts_len = 0;
    int *replacement = NULL;

    for (size_t i = 0; i < template.num_fmt_strings; i++) {
        if (template.replacement_ids[i] == _UNDEFINED) {
            size_t fmt_len = strlen(template.fmt_strings[i]);
            result_parts_len += fmt_len;
            result_parts[i] = malloc(sizeof(char*) * fmt_len + 1);
            strcpy(result_parts[i], template.fmt_strings[i]);
        } else {
            switch (template.replacement_ids[i]) {
            case REPRESENTATION_ID:
                replacement = &representation_id;
                break;
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
            }
            result_parts_len += asprintf(&result_parts[i], template.fmt_strings[i], *replacement);
        }
    }

    char *result = malloc(result_parts_len * sizeof (char*) + 1);
    size_t result_len = 0;
    for (size_t i = 0; i < template.num_fmt_strings; i++) {
        for (size_t j = 0; result_parts[i][j] != '\0'; j++) {
            result[result_len++] = result_parts[i][j];
        }
        free(result_parts[i]);
    }
    result[result_len] = '\0';

    return result;
}

struct SegmentTime {
    int start;
    int part_duration;
    int part_count;
};

struct SegmentTemplate {
    const char *initialization;
    const char *media;
    struct Vector *timeline;
};

struct Representation {
    const char *id;
    struct SegmentTemplate segment_template;
    // TODO(Jacques): Allow Representation to override AdaptationSet.SegmentTemplate
};

struct AdaptationSet {
    const char *mime_type;
    struct SegmentTemplate segment_template;
    struct Vector *representations;
};

struct SegmentTemplate get_segment_template(mxml_node_t *adaptation_set)
{
    struct SegmentTemplate template = {0};
    template.timeline = vector_init();

    mxml_node_t *root = mxmlFindElement(adaptation_set, adaptation_set, "SegmentTemplate", NULL, NULL, MXML_DESCEND_FIRST);
    template.initialization = mxmlElementGetAttr(root, "initialization");
    template.media = mxmlElementGetAttr(root, "media");

	int last_end = 0;
    mxml_node_t *timeline = mxmlFindElement(root, root, "SegmentTimeline", NULL, NULL, MXML_DESCEND_FIRST);
    for (
        mxml_node_t *node = mxmlFindElement(timeline, timeline, "S", NULL, NULL, MXML_DESCEND_FIRST);
        node != NULL;
        node = mxmlFindElement(node, timeline, "S", NULL, NULL, MXML_DESCEND_FIRST)
    ) {
        struct SegmentTime *t = malloc(sizeof (struct SegmentTime));
        const char *a;

        t->part_duration = atoi(mxmlElementGetAttr(node, "d"));

        if ((a = mxmlElementGetAttr(node, "t")) != NULL) {
            t->start = atoi(a);
        } else {
            t->start = last_end;
        }
        if ((a = mxmlElementGetAttr(node, "r")) != NULL) {
            t->part_count = atoi(a);
        } else {
            t->part_count = 1;
        }

        last_end = t->start + t->part_duration * t->part_count;
        template.timeline = vector_append(template.timeline, t);
    }

    return template;
}

struct Vector *get_adaptation_sets(mxml_node_t *root)
{
    const char *TAG_ADAPTATION_SET = "AdaptationSet";
    const char *TAG_REPRESENTATION = "Representation";
    struct AdaptationSet *set;
    struct Vector *sets = vector_init();

    for (
        mxml_node_t *anode = mxmlFindElement(root, root, TAG_ADAPTATION_SET, NULL, NULL, MXML_DESCEND);
        anode != NULL;
        anode = mxmlFindElement(anode, root, TAG_ADAPTATION_SET, NULL, NULL, MXML_DESCEND)
    ) {
        set = malloc(sizeof (struct AdaptationSet));
        set->representations = vector_init();
        set->mime_type = mxmlElementGetAttr(anode, "mimeType");
        set->segment_template = get_segment_template(anode);

        for (
            mxml_node_t *rnode = mxmlFindElement(anode, anode, TAG_REPRESENTATION, NULL, NULL, MXML_DESCEND_FIRST);
            rnode != NULL;
            rnode = mxmlFindElement(rnode, anode, TAG_REPRESENTATION, NULL, NULL, MXML_DESCEND_FIRST)
        ) {
            struct Representation *r = malloc(sizeof (struct Representation));
            r->id = mxmlElementGetAttr(rnode, "id");
            set->representations = vector_append(set->representations, r);
        }

        sets = vector_append(sets, set);
    }

    return sets;
}

int main(int argc, char *argv[argc + 1])
{
    if (argc != 2) {
        printf("No URL specified\n");
        return 1;
    }

    struct String manifeststr;
    const char *url = argv[1];
    mxml_node_t *root;
    struct Vector *adaptation_sets;

    curl_global_init(0);

    int ret = http_get_as_string(&manifeststr, url);
    printf("%s", manifeststr.data);

    root = mxmlLoadString(NULL, manifeststr.data, MXML_OPAQUE_CALLBACK);
    adaptation_sets = get_adaptation_sets(root);
	for (size_t i = 0; i < adaptation_sets->len; i++) {
        struct AdaptationSet *set = (struct AdaptationSet *)adaptation_sets->items[i];
        printf("%zu\t%s\n", i, set->mime_type);
        printf("initialization: %s media: %s timeline_len: %zu\n", set->segment_template.initialization, set->segment_template.media, set->segment_template.timeline->len);
        struct URLTemplate media_template = parse_url_template(set->segment_template.media);
        char *media = url_template_format(media_template, 1, 2, 3, 4);
        printf("media: %s\n", media);
        free(media);
        for (size_t j = 0; j < set->segment_template.timeline->len; j++) {
            struct SegmentTime *t = set->segment_template.timeline->items[j];
            printf("start: %d duration: %d count: %d\n", t->start, t->part_duration, t->part_count);
        }
        for (size_t j = 0; j < set->representations->len; j++) {
            struct Representation *r = set->representations->items[j];
            printf("representation_id: %s\n", r->id);
        }
    }
    vector_free(adaptation_sets);
    // TODO(Jacques): Free vectors: representations and timeline

    mxmlDelete(root);
    free(manifeststr.data);

    curl_global_cleanup();

    return ret ? 0 : 2;
}
