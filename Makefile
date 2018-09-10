CC = gcc
CFLAGS = -pedantic -std=c11 -O3 -fstrict-aliasing -fsanitize=undefined
CFLAGS += -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing
DEBUG = 1

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -DNDEBUG
endif

streamcat: streamcat.c streamlisting.c output.c
	$(CC) $(CFLAGS) -lcurl output.c streamlisting.c streamcat.c -o$@

mpdcat: mpdcat.c muxing.c muxing.h mpd.h mpd.c http.h http.c vector.h vector.c
	$(CC) $(CFLAGS) -lavcodec -lavformat -lavutil -lcurl -lmxml muxing.c mpd.c http.c output.c vector.c mpdcat.c -o$@

test: http_test.c http.c mpd_test.c vector.c vector.h vector_test.c http.h output.c vendor/scut/scut.c vendor/scut/scut.h unittest.c
	$(CC) $(CFLAGS) -lcurl -lmxml http_test.c http.c vendor/scut/scut.c mpd_test.c mpd.c unittest.c output.c vector.c vector_test.c -o$@
	valgrind ./$@
	rm $@
