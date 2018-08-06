#include <stdarg.h>
#include <stdio.h>

int _dbg_print(const char *subject, const char *filename, int line, const char *format, ...)
{
    va_list argptr;
    int n_bytes;

	va_start(argptr, format);
    n_bytes = fprintf(stderr, "%s(%s:%d): ", subject, filename, line);
    n_bytes += vfprintf(stderr, format, argptr);
    va_end(argptr);

    return n_bytes;
}
