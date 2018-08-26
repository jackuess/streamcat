#ifndef vector2_h_INCLUDED
#define vector2_h_INCLUDED

void *vecnew(size_t nmemb, size_t item_size);
void *vecappend(void *dest, const void* src);
void *vecextend(void *dest, const void* src, size_t n);
size_t veclen(const void *data);
void* vecsetlen(void *data, size_t newlen);
void vecfree(void *data);

#endif // vector2_h_INCLUDED
