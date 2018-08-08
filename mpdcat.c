#include <stdio.h>
#include <stdlib.h>

#include <curl.h>
#include <mxml.h>

#include "http.h"
#include "output.h"
#include "vector.h"

struct AdaptationSet {
    int id;
    const char *mime_type;
};

struct Vector *get_adaptation_sets(mxml_node_t *root)
{
    const char *TAGNAME = "AdaptationSet";
    struct AdaptationSet *set;
    struct Vector *sets = vector_init();

    for (
        mxml_node_t *node = mxmlFindElement(root, root, TAGNAME, NULL, NULL, MXML_DESCEND);
        node != NULL;
        node = mxmlFindElement(node, root, TAGNAME, NULL, NULL, MXML_DESCEND)
    ) {
        set = malloc(sizeof (struct AdaptationSet));
        set->mime_type = mxmlElementGetAttr(node, "mimeType");
        set->id = atoi(mxmlElementGetAttr(node, "id"));
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
        printf("%d\t%s\n", set->id, set->mime_type);
    }
    vector_free(adaptation_sets);

    mxmlDelete(root);
    free(manifeststr.data);

    curl_global_cleanup();

    return ret ? 0 : 2;
}
