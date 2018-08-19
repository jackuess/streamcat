#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>

#include "streamlisting.h"

int main(int argc, char *argv[argc+1])
{
	struct StreamList *stream_list;
	struct StreamList *chunks;
	int o;
	int showinfo = 0;
	int download = 1;
	long stream_index = -1;
	const char *url;

	while ((o = getopt(argc, argv, "i:vI")) != -1) {
    	switch (o) {
        case 'i':
            stream_index = strtol(optarg, NULL, 10);
        	break;
    	case 'v':
        	showinfo = 1;
        	break;
        case 'I':
        	showinfo = 1;
        	download = 0;
        	break;
    	}
	};
	if (optind >= argc) {
    	printf("No URL specified\n");
    	return 1;
	}
	url = argv[optind];

    curl_global_init(0);

    if (!(stream_list = m3u8_get_stream_list(url))) {
        printf("Unknown stream list format\n");
        return 2;
    }
    if (showinfo) {
        FILE *f = stderr;
        if (!download) {
            f = stdout;
        }
        stream_list_print(f, stream_list);
    }

    if (download) {
        if (stream_index < 0 || stream_index >= (int)stream_list->n_streams) {
            stream_index = stream_list->n_streams - 1;
        }
        if (!(chunks = m3u8_get_stream_list(stream_list->streams[stream_index]))) {
            printf("No chunks found\n");
            stream_list_free(stream_list);
            return 3;
        }

        concat_streams(stdout, chunks);
        stream_list_free(chunks);
    }
    stream_list_free(stream_list);

    curl_global_cleanup();

    return 0;
}
