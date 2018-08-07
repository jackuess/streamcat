#include <stdio.h>
#include <stdlib.h>

#include "curl.h"

#include "http.h"

int main(int argc, char *argv[argc + 1])
{
    struct String manifeststr;
    if (argc != 2) {
        printf("No URL specified\n");
        return 1;
    }
    const char *url = argv[1];

    curl_global_init(0);

    int ret = http_get_as_string(&manifeststr, url);
    printf("%s", manifeststr.data);
    free(manifeststr.data);

    curl_global_cleanup();

    return ret ? 0 : 2;
}
