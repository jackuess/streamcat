#ifndef vector2_h_INCLUDED
#define vector2_h_INCLUDED

void *vecnew(size_t nmemb, size_t item_size);
void *vecappend(void **dest, const void* src);
#define VECAPPEND(dest, src) vecappend((void**)dest, src)
void *vecextend(void **dest, const void* src, size_t n);
#define VECEXTEND(dest, src, n) vecextend((void**)dest, src, n)
size_t veclen(const void *data);
void vecfree(void *data);

#endif // vector2_h_INCLUDED
