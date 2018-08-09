#include <stdio.h>
#include <stdlib.h>

#include <curl.h>
#include <mxml.h>

#include "http.h"
#include "output.h"
#include "vector.h"

struct SegmentTime {
    int start;
    int part_duration;
    int part_count;
};

struct Representation {
    const char *id;
};

struct SegmentTemplate {
    const char *initialization;
    const char *media;
    struct Vector *timeline;
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
