#ifndef output_h_INCLUDED
#define output_h_INCLUDED

int _dbg_print(const char *, const char *, int, const char *, ...);
#define dbg_print(subject, ...) \
    _dbg_print(subject, __FILE__, __LINE__, __VA_ARGS__)

#endif  // output_h_INCLUDED
