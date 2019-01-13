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
SRC_DIR = src
TEST_DIR = test

.PHONY: build clean indent memcheck scan test

all: $(BINARIES)

STREAMCAT_LIBS = -lavcodec -lavformat -lavutil -lcurl -lmxml
STREAMCAT_HEADERS = \
    $(SRC_DIR)/codec.c \
    $(SRC_DIR)/hls.h \
    $(SRC_DIR)/http.h \
    $(SRC_DIR)/muxing.h \
    $(SRC_DIR)/mpd.h \
    $(SRC_DIR)/output.h \
    $(SRC_DIR)/streamlisting.h \
    vendor/arr/arr.h
STREAMCAT_SOURCES = \
    $(SRC_DIR)/codec.c \
    $(SRC_DIR)/hls.c \
    $(SRC_DIR)/http.c \
    $(SRC_DIR)/muxing.c \
    $(SRC_DIR)/mpd.c \
    $(SRC_DIR)/output.c \
    $(SRC_DIR)/streamlisting.c \
    vendor/arr/arr.c \
    $(SRC_DIR)/streamcat.c
$(BINARY_DIR)/streamcat: $(STREAMCAT_HEADERS) $(STREAMCAT_SOURCES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(STREAMCAT_LIBS) $(STREAMCAT_SOURCES) -o$@

MPDCAT_LIBS = -lavcodec -lavformat -lavutil -lcurl -lmxml
MPDCAT_HEADERS = \
    $(SRC_DIR)/codec.h \
    $(SRC_DIR)/http.h \
    $(SRC_DIR)/mpd.h \
    $(SRC_DIR)/muxing.h \
    $(SRC_DIR)/output.h \
    vendor/arr/arr.h
MPDCAT_SOURCES = \
    $(SRC_DIR)/codec.c \
    $(SRC_DIR)/http.c \
    $(SRC_DIR)/mpd.c \
    $(SRC_DIR)/muxing.c \
    $(SRC_DIR)/output.c \
    vendor/arr/arr.c \
    $(SRC_DIR)/mpdcat.c
$(BINARY_DIR)/mpdcat: $(MPDCAT_HEADERS) $(MPDCAT_SOURCES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(MPDCAT_LIBS) $(MPDCAT_SOURCES) -o$@

TEST_SOURCES = \
    $(TEST_DIR)/test_*.c \
    $(SRC_DIR)/codec.c \
    $(SRC_DIR)/hls.c \
    $(SRC_DIR)/http.c \
    $(SRC_DIR)/mpd.c \
    $(SRC_DIR)/output.c \
    $(SRC_DIR)/streamlisting.c \
    vendor/arr/arr.c
$(BINARY_DIR)/test: $(TEST_SOURCES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -lcurl -lmxml vendor/scut/scut.c $(TEST_DIR)/unittest.c $(TEST_SOURCES) -o$@

test: $(BINARY_DIR)/test
	$<

memcheck: $(BINARY_DIR)/test
	valgrind $<

indent:
	clang-format -i -style=file *.h *.c

clean:
	rm -r $(BINARY_DIR)/

scan: clean
	scan-build $(MAKE) test
