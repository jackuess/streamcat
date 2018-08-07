#ifndef http_h_INCLUDED
#define http_h_INCLUDED

struct String {
    char *data;
    size_t size;
};

void string_init(struct String *str);
int http_get_as_string(struct String *dest, const char *url);

#endif // http_h_INCLUDED

