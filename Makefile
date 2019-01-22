CC = gcc
CFLAGS = -pedantic -std=c11 -O3 -fstrict-aliasing -fsanitize=undefined
CFLAGS += -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing
DEBUG = 1

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -DNDEBUG
endif

OBJ_DIR = obj
SHARED_OBJ_DIR = $(OBJ_DIR)/shared
SO_NAME = libstreamcat.so.1
BINARIES = mpdcat streamcat libstreamcat.a $(SO_NAME)
SRC_DIR = src
TEST_DIR = test

.PHONY: build clean indent memcheck scan test

all: $(SO_NAME) $(BINARIES)

VENDOR_SOURCES = \
    vendor/arr/arr.c

LIBS = -lavcodec -lavformat -lavutil -lcurl -lmxml
SOURCES = \
    $(SRC_DIR)/codec.c \
    $(SRC_DIR)/hls.c \
    $(SRC_DIR)/http.c \
    $(SRC_DIR)/muxing.c \
    $(SRC_DIR)/mpd.c \
    $(SRC_DIR)/output.c \
    $(SRC_DIR)/streamlisting.c

$(SHARED_OBJ_DIR)/arr.o: vendor/arr/arr.c
	@mkdir -p $(@D)
	$(CC) -c -fPIC $(CFLAGS) $< -o$@
$(SHARED_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c -fPIC $(CFLAGS) $< -o$@

SHARED_OBJS = \
    $(patsubst $(SRC_DIR)/%.c,$(SHARED_OBJ_DIR)/%.o,$(SOURCES)) \
    $(SHARED_OBJ_DIR)/arr.o
$(SO_NAME): $(SHARED_OBJS)
	@mkdir -p $(@D)
	$(CC) -shared -fPIC -Wl,-soname,$(SO_NAME) -o $@ $(SHARED_OBJS) -lc

$(OBJ_DIR)/arr.o: vendor/arr/arr.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o$@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o$@
STATIC_OBJS = \
    $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES)) \
    $(OBJ_DIR)/arr.o

libstreamcat.a: $(STATIC_OBJS)
	ar -rcs $@ $(STATIC_OBJS)

streamcat: libstreamcat.a $(SRC_DIR)/streamcat.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LIBS) $(VENDOR_SOURCES) $(SRC_DIR)/streamcat.c -L. -lstreamcat -o$@

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
mpdcat: $(MPDCAT_HEADERS) $(MPDCAT_SOURCES)
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
$(TEST_DIR)/test: $(TEST_SOURCES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -lcurl -lmxml vendor/scut/scut.c $(TEST_DIR)/unittest.c $(TEST_SOURCES) -o$@

test: $(TEST_DIR)/test
	$<

memcheck: $(TEST_DIR)/test
	valgrind $<

indent:
	clang-format -i -style=file $(SRC_DIR)/*.h $(SRC_DIR)/*.c

clean:
	-rm $(BINARIES)
	-rm $(TEST_DIR)/test
	-rm -r $(OBJ_DIR)

scan: clean
	scan-build $(MAKE) test
