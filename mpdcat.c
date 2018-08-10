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
    char *template;
    enum URL_TEMPLATE_IDENTIFIERS replacement_ids[MAX_REPLACEMENT_IDS];
};

struct URLTemplate parse_url_template(const char *str)
{
    bool tag_open = false;
    bool format_open = false;
    enum URL_TEMPLATE_IDENTIFIERS replacement_tag = _UNDEFINED;
    size_t num_replacement_ids = 0;
    struct URLTemplate template = {0};
    template.template = malloc(sizeof (template.template) * strlen(str) + 1);
    template.replacement_ids[0] = _UNDEFINED;

	size_t j = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '$') {
            if (tag_open) {
                template.replacement_ids[num_replacement_ids++] = replacement_tag;
                if (!format_open) {
                    template.template[j++] = '%';
                    template.template[j++] = 'd';
                }

                replacement_tag = _UNDEFINED;
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
                template.template[j++] = str[i];
            } else if (str[i] == '%') {
				template.template[j++] = '%';
				format_open = true;
            }
        } else {
            template.template[j++] = str[i];
        }
    }
    template.template[j] = '\0';
    template.replacement_ids[num_replacement_ids] = _UNDEFINED;

    return template;
}

char *url_template_format(const char *template, int representation_id, int number, int bandwidth, int time)
{
    bool tag_open = false;
    bool format_open = false;
    int *replacement_tag = NULL;
    size_t result_capacity = sizeof (char*) * strlen(template) * 2;
    char *result = malloc(result_capacity);
    size_t result_len = 0;
    char *tail = malloc(sizeof (char*) * 3);
    size_t tail_len = 0;
    char *fmt = malloc(sizeof(char*) * 1024);  // TODO(Jacques): Make dynamic
    size_t fmt_len = 0;

    for (size_t i = 0; template[i] != '\0'; i++) {
        if (template[i] == '$') {
            if (tag_open) {
                if (replacement_tag == NULL) {
                    tail[tail_len++] = '$';
                } else {
                    if (format_open) {
                        fmt[fmt_len] = '\0';
                    } else {
                        strcpy(fmt, "%d");
                    }
                    if (replacement_tag != NULL) {
                        tail_len += asprintf(&tail, fmt, *replacement_tag);
                    }

                    replacement_tag = NULL;
                }
                tag_open = false;
            } else {
                tag_open = true;
                format_open = false;
            }
        } else if (tag_open) {
            if (replacement_tag == NULL) {
                switch (template[i]) {
                case 'R':
                    replacement_tag = &representation_id;
                    break;
                case 'N':
                    replacement_tag = &number;
                    break;
                case 'B':
                    replacement_tag = &bandwidth;
                    break;
                case 'T':
                    replacement_tag = &time;
                    break;
                }
            } else if (format_open) {
                fmt[fmt_len++] = template[i];
            } else if (template[i] == '%') {
                fmt[fmt_len++] = '%';
				format_open = true;
            }
        } else {
            tail[tail_len++] = template[i];
        }

        if ((result_capacity / sizeof (char*)) < (result_len + tail_len + 1)) {
            result_capacity *= 2;
            result = realloc(result, result_capacity);
        }
        for (size_t j = 0; j < tail_len; j++) {
            result[result_len++] = tail[j];
        }
        tail_len = 0;
    }
    free(fmt);
    free(tail);

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
        printf("media: %s %d\n", media_template.template, media_template.replacement_ids[1]);
        printf("media: %s\n", url_template_format(set->segment_template.media, 1, 2, 3, 4));
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
