#ifndef vector_h_INCLUDED
#define vector_h_INCLUDED

#include <stdalign.h>

void *vecnew(size_t nmemb, size_t item_size, size_t alignment);
void *vecappend(void **dest, size_t item_size, size_t alignment);
void *vecextend(void **dest, size_t item_size, size_t alignment, size_t n);
size_t veclen(const void *data);
void vecfree(void *data, size_t alignment);

#define VECNEW(nmemb, type) vecnew(nmemb, sizeof (type), alignof (type))
#define VECAPPEND(dest) vecappend((void**)dest, sizeof (*dest[0]), alignof (*dest[0]))
#define VECEXTEND(dest, n) vecextend((void**)dest, sizeof (*dest[0]), alignof (*dest[0]), n)
#define VECFREE(data) vecfree(data, alignof (data[0]))

#endif // vector_h_INCLUDED
