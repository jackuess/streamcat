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

mpdcat: mpdcat.c mpd.h mpd.c http.h http.c vector2.h
	$(CC) $(CFLAGS) -lcurl -lmxml mpd.c http.c output.c mpdcat.c -o$@

muxer: muxer.c
	$(CC) $(CFLAGS) -lavformat -lavcodec -lavutil muxer.c -o$@

test: http_test.c http.c mpd_test.c http.h output.c minunit.h unittest.c
	$(CC) $(CFLAGS) -lcurl -lmxml http_test.c http.c mpd_test.c mpd.c unittest.c output.c -o$@
	./$@
	rm $@
