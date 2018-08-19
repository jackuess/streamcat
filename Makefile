CC = gcc
CFLAGS = -pedantic -std=c99 -O3 -fstrict-aliasing
CFLAGS += -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing
DEBUG = 1

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -DNDEBUG
endif

streamcat: streamcat.c streamlisting.c output.c
	$(CC) $(CFLAGS) -lcurl output.c streamlisting.c streamcat.c -o$@

mpdcat: mpdcat.c http.h http.c vector2.h
	$(CC) $(CFLAGS) -lcurl -lmxml http.c output.c mpdcat.c -o$@

test: http_test.c http.c http.h output.c
	$(CC) $(CFLAGS) -lcurl http_test.c http.c output.c -o$@
	./$@
	rm $@
