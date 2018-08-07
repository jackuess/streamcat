#ifndef streamlisting_h_INCLUDED
#define streamlisting_h_INCLUDED

struct StreamList {
    unsigned int n_streams;
    char *streams[1000];  // TODO(Jacques): Make dynamic
};

void stream_list_free(struct StreamList *lst);
struct StreamList *m3u8_get_stream_list(const char *url);
int concat_streams(FILE *stream, const struct StreamList *lst);

#endif // streamlisting_h_INCLUDED

