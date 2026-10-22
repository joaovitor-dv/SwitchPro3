CC ?= cc
AR ?= ar
CFLAGS ?= -O2
CPPFLAGS ?=

WARNINGS := -Wall -Wextra -Wpedantic -Werror
STANDARD := -std=c11
INCLUDES := -Iinclude
BUILD_DIR := build
LIBRARY := $(BUILD_DIR)/libswitchpro3.a
TEST_BINARY := $(BUILD_DIR)/test_hid

.PHONY: all test clean

all: $(LIBRARY)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/hid.o: src/hid.c include/switchpro3/hid.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(STANDARD) $(WARNINGS) $(CFLAGS) -c $< -o $@

$(LIBRARY): $(BUILD_DIR)/hid.o
	$(AR) rcs $@ $^

$(TEST_BINARY): tests/test_hid.c $(LIBRARY)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(STANDARD) $(WARNINGS) $(CFLAGS) $(LDFLAGS) $< $(LIBRARY) $(LDLIBS) -o $@

test: $(TEST_BINARY)
	$(TEST_BINARY)

clean:
	$(RM) -r $(BUILD_DIR)

