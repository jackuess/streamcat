CC = gcc
CFLAGS = -pedantic -std=c11 -O3 -fstrict-aliasing -fsanitize=undefined
CFLAGS += -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing
DEBUG = 1

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -DNDEBUG
endif

BINARY_DIR = bin
BINARIES = $(BINARY_DIR)/mpdcat $(BINARY_DIR)/streamcat

.PHONY: build test memcheck indent

build: $(BINARIES)

$(BINARY_DIR):
	mkdir $@

STREAMCAT_LIBS = -lcurl
STREAMCAT_HEADERS = output.h streamlisting.h
STREAMCAT_SOURCES = output.c streamlisting.c streamcat.c
$(BINARY_DIR)/streamcat: $(BINARY_DIR) $(STREAMCAT_HEADERS) $(STREAMCAT_SOURCES)
	$(CC) $(CFLAGS) $(STREAMCAT_LIBS) $(STREAMCAT_SOURCES) -o$@

MPDCAT_LIBS = -lavcodec -lavformat -lavutil -lcurl -lmxml
MPDCAT_HEADERS = http.h mpd.h muxing.h output.h vendor/arr/arr.h
MPDCAT_SOURCES = http.c mpd.c muxing.c output.c vendor/arr/arr.c mpdcat.c
$(BINARY_DIR)/mpdcat: $(BINARY_DIR) $(MPDCAT_HEADERS) $(MPDCAT_SOURCES)
	$(CC) $(CFLAGS) $(MPDCAT_LIBS) $(MPDCAT_SOURCES) -o$@

TEST_SOURCES = \
    *_test.c \
    http.c \
    mpd.c \
    output.c \
    vendor/arr/arr.c
$(BINARY_DIR)/test: $(BINARY_DIR) $(TEST_SOURCES)
	$(CC) $(CFLAGS) -lcurl -lmxml vendor/scut/scut.c unittest.c $(TEST_SOURCES) -o$@

test: $(BINARY_DIR)/test
	$<

memcheck: $(BINARY_DIR)/test
	valgrind $<

indent:
	clang-format -i -style=file *.h *.c
