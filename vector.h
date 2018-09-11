#ifndef vector_h_INCLUDED
#define vector_h_INCLUDED

#include <stddef.h>

void *vecnew(size_t nmemb, size_t item_size);
void *vecappend(void **dest);
void *vecextend(void **dest, size_t n);
size_t veclen(const void *data);
void vecfree(void *data);

#define VECAPPEND(dest) vecappend((void **)dest)
#define VECEXTEND(dest, n) vecextend((void **)dest, n)

#endif  // vector_h_INCLUDED
