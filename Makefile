CC = gcc
CFLAGS = -pedantic -std=c11 -O3 -fstrict-aliasing -fsanitize=undefined
CFLAGS += -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing
DEBUG = 1

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -DNDEBUG
endif

bin/:
	mkdir $@

STREAMCAT_LIBS = -lcurl
STREAMCAT_HEADERS = output.h streamlisting.h
STREAMCAT_SOURCES = output.c streamlisting.c streamcat.c
bin/streamcat: bin/ $(STREAMCAT_HEADERS) $(STREAMCAT_SOURCES)
	$(CC) $(CFLAGS) $(STREAMCAT_LIBS) $(STREAMCAT_SOURCES) -o$@
.PHONY: streamcat
streamcat: bin/streamcat

MPDCAT_LIBS = -lavcodec -lavformat -lavutil -lcurl -lmxml
MPDCAT_HEADERS = http.h mpd.h muxing.h output.h vendor/arr/arr.h
MPDCAT_SOURCES = http.c mpd.c muxing.c output.c vendor/arr/arr.c mpdcat.c
bin/mpdcat: bin/ $(MPDCAT_HEADERS) $(MPDCAT_SOURCES)
	$(CC) $(CFLAGS) $(MPDCAT_LIBS) $(MPDCAT_SOURCES) -o$@
.PHONY: mpdcat
mpdcat: bin/mpdcat

TEST_SOURCES = \
    *_test.c \
    http.c \
    mpd.c \
    output.c \
    vendor/arr/arr.c
bin/test: bin/ $(TEST_SOURCES)
	$(CC) $(CFLAGS) -lcurl -lmxml vendor/scut/scut.c unittest.c $(TEST_SOURCES) -o$@

.PHONY: test
test: bin/test
	$<

.PHONY: memcheck
memcheck: bin/test
	valgrind $<

.PHONY: indent
indent:
	clang-format -i -style=file *.h *.c
