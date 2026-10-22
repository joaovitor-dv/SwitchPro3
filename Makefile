CC ?= cc
AR ?= ar
CFLAGS ?= -O2
CPPFLAGS ?=

WARNINGS := -Wall -Wextra -Wpedantic -Werror
STANDARD := -std=c11
INCLUDES := -Iinclude
BUILD_DIR := build
LIBRARY := $(BUILD_DIR)/libswitchpro3.a
TEST_BINARIES := $(BUILD_DIR)/test_hid $(BUILD_DIR)/test_calibration
HEADERS := $(wildcard include/switchpro3/*.h)

.PHONY: all test clean

all: $(LIBRARY)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(STANDARD) $(WARNINGS) $(CFLAGS) -c $< -o $@

$(LIBRARY): $(BUILD_DIR)/hid.o $(BUILD_DIR)/calibration.o
	$(AR) rcs $@ $^

$(BUILD_DIR)/test_hid: tests/test_hid.c $(LIBRARY)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(STANDARD) $(WARNINGS) $(CFLAGS) $(LDFLAGS) $< $(LIBRARY) $(LDLIBS) -o $@

$(BUILD_DIR)/test_calibration: tests/test_calibration.c tests/fixture_loader.h $(LIBRARY)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(STANDARD) $(WARNINGS) $(CFLAGS) $(LDFLAGS) $< $(LIBRARY) $(LDLIBS) -o $@

test: $(TEST_BINARIES)
	$(BUILD_DIR)/test_hid
	$(BUILD_DIR)/test_calibration

clean:
	$(RM) -r $(BUILD_DIR)

