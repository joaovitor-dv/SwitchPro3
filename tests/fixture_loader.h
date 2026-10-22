/* SPDX-License-Identifier: MIT */
#ifndef SWITCHPRO3_TEST_FIXTURE_LOADER_H
#define SWITCHPRO3_TEST_FIXTURE_LOADER_H

/*
 * Test-only loader for recorded-report fixtures: one report per non-empty line,
 * tokens of exactly two hex digits separated by spaces or tabs, '#' starts a
 * comment. Any violation fails the whole load; no partial data is returned.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FIXTURE_MAX_LINE 1024u
#define FIXTURE_MAX_REPORT 64u

struct fixture_report {
    uint8_t data[FIXTURE_MAX_REPORT];
    size_t length;
};

static int fixture_hex_value(const char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int fixture_fail(char *error, const size_t error_size, const char *name,
                        const size_t line, const char *message)
{
    if (error != NULL && error_size > 0) {
        (void)snprintf(error, error_size, "%s:%zu: %s", name, line, message);
    }
    return -1;
}

/* Returns 0 on success, -1 on error with "name:line: message" in `error`. */
static int fixture_parse(const char *name, const char *text,
                         struct fixture_report *reports, const size_t capacity,
                         size_t *count, char *error, const size_t error_size)
{
    size_t line_number = 1;
    size_t found = 0;
    const char *cursor = text;

    while (*cursor != '\0') {
        const char *end = cursor;
        const char *p;
        struct fixture_report report;

        while (*end != '\0' && *end != '\n') {
            end++;
        }
        if ((size_t)(end - cursor) > FIXTURE_MAX_LINE) {
            return fixture_fail(error, error_size, name, line_number, "line too long");
        }

        report.length = 0;
        p = cursor;
        while (p < end && *p != '#') {
            const char *token;
            size_t token_length;

            if (*p == ' ' || *p == '\t' || *p == '\r') {
                p++;
                continue;
            }
            token = p;
            while (p < end && *p != '#' && *p != ' ' && *p != '\t' && *p != '\r') {
                p++;
            }
            token_length = (size_t)(p - token);
            if (token_length != 2 || fixture_hex_value(token[0]) < 0 ||
                fixture_hex_value(token[1]) < 0) {
                return fixture_fail(error, error_size, name, line_number,
                                    "token is not two hex digits");
            }
            if (report.length >= FIXTURE_MAX_REPORT) {
                return fixture_fail(error, error_size, name, line_number,
                                    "report longer than 64 bytes");
            }
            report.data[report.length++] =
                (uint8_t)((fixture_hex_value(token[0]) << 4) | fixture_hex_value(token[1]));
        }

        if (report.length > 0) {
            if (found >= capacity) {
                return fixture_fail(error, error_size, name, line_number,
                                    "too many reports");
            }
            reports[found++] = report;
        }

        if (*end == '\0') {
            break;
        }
        cursor = end + 1;
        line_number++;
    }

    if (found == 0) {
        return fixture_fail(error, error_size, name, line_number, "no reports");
    }
    *count = found;
    return 0;
}

/* Reads a whole file (at most 64 KiB) and parses it. */
static int fixture_load_file(const char *path, struct fixture_report *reports,
                             const size_t capacity, size_t *count, char *error,
                             const size_t error_size)
{
    static char buffer[65536];
    FILE *file = fopen(path, "rb");
    size_t length;

    if (file == NULL) {
        return fixture_fail(error, error_size, path, 0, "cannot open file");
    }
    length = fread(buffer, 1, sizeof(buffer) - 1, file);
    if (ferror(file) != 0 || !feof(file)) {
        (void)fclose(file);
        return fixture_fail(error, error_size, path, 0, "cannot read file completely");
    }
    (void)fclose(file);
    buffer[length] = '\0';
    if (memchr(buffer, '\0', length) != NULL) {
        return fixture_fail(error, error_size, path, 0, "file contains a NUL byte");
    }
    return fixture_parse(path, buffer, reports, capacity, count, error, error_size);
}

#endif
