#include <stdio.h>

#include <curl.h>

#include "streamlisting.h"

int main(int argc, char *argv[argc+1])
{
	struct StreamList *stream_list;
	struct StreamList *chunks;

    if (argc != 2) {
        return 1;
    }

    curl_global_init(0);

    if (!(stream_list = m3u8_get_stream_list(argv[1]))) {
        printf("Unknown stream list format\n");
        return 2;
    }
    if (!(chunks = m3u8_get_stream_list(stream_list->streams[0]))) {
        printf("No chunks found\n");
        stream_list_free(stream_list);
        return 3;
    }

    concat_streams(stdout, chunks);
    stream_list_free(chunks);
    stream_list_free(stream_list);

    curl_global_cleanup();

    return 0;
}
