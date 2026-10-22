/* SPDX-License-Identifier: MIT */
#include "switchpro3/calibration.h"
#include "switchpro3/hid.h"

#include "fixture_loader.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define EXPECT_TRUE(expression) \
    do { \
        if (!(expression)) { \
            fprintf(stderr, "%s:%d: expectation failed: %s\n", \
                    __FILE__, __LINE__, #expression); \
            failures++; \
        } \
    } while (0)

#define SENTINEL ((int16_t)12345)

/* One calibrate call; on error *out must keep its sentinel value. */
static void expect_cal(const struct sp3_axis_calibration *cal, const uint16_t raw,
                       const enum sp3_cal_status status, const int expected,
                       const int line)
{
    int16_t out = SENTINEL;
    const enum sp3_cal_status got = sp3_axis_calibrate(cal, raw, &out);

    if (got != status) {
        fprintf(stderr, "line %d: raw %u status %d, want %d\n", line,
                (unsigned int)raw, (int)got, (int)status);
        failures++;
        return;
    }
    if (status == SP3_CAL_OK ? out != expected : out != SENTINEL) {
        fprintf(stderr, "line %d: raw %u out %d, want %d\n", line,
                (unsigned int)raw, (int)out,
                status == SP3_CAL_OK ? expected : (int)SENTINEL);
        failures++;
    }
}

#define CAL_OK(cal, raw, expected) expect_cal(cal, raw, SP3_CAL_OK, expected, __LINE__)
#define CAL_ERR(cal, raw, status) expect_cal(cal, raw, status, 0, __LINE__)

static const struct sp3_axis_calibration asym0 = { 500, 2000, 3800, 0 };
static const struct sp3_axis_calibration asym100 = { 500, 2000, 3800, 100 };
static const struct sp3_axis_calibration sym0 = { 1, 2048, 4095, 0 };
static const struct sp3_axis_calibration sym100 = { 1, 2048, 4095, 100 };

static void test_fixed_points_and_saturation(void)
{
    CAL_OK(&asym0, 2000, 0);
    CAL_OK(&asym0, 500, -32767);
    CAL_OK(&asym0, 3800, 32767);
    CAL_OK(&asym0, 499, -32767);   /* below min, still a valid sample */
    CAL_OK(&asym0, 3801, 32767);   /* above max, still a valid sample */
    CAL_OK(&asym0, 0, -32767);
    CAL_OK(&asym0, 4095, 32767);

    CAL_OK(&asym100, 2000, 0);
    CAL_OK(&asym100, 500, -32767);
    CAL_OK(&asym100, 3800, 32767);
    CAL_OK(&asym100, 499, -32767);
    CAL_OK(&asym100, 3801, 32767);

    CAL_OK(&sym0, 2048, 0);
    CAL_OK(&sym0, 1, -32767);
    CAL_OK(&sym0, 4095, 32767);
    CAL_OK(&sym0, 0, -32767);      /* min - 1, valid sample */

    CAL_OK(&sym100, 2048, 0);
    CAL_OK(&sym100, 1, -32767);
    CAL_OK(&sym100, 4095, 32767);
    CAL_OK(&sym100, 0, -32767);
}

static void test_deadzone_edges(void)
{
    /* deadzone == 0: only 0 and +-1 from center (deadzone - 1 is undefined). */
    CAL_OK(&asym0, 2000, 0);
    CAL_OK(&asym0, 2001, 18);      /* 1 * 32767 / 1800 */
    CAL_OK(&asym0, 1999, -21);     /* 1 * 32767 / 1500 */
    CAL_OK(&sym0, 2049, 16);       /* 1 * 32767 / 2047 */
    CAL_OK(&sym0, 2047, -16);

    /* deadzone == 100: distance 99 and 100 give 0, 101 is the first step. */
    CAL_OK(&asym100, 2099, 0);
    CAL_OK(&asym100, 2100, 0);
    CAL_OK(&asym100, 2101, 19);    /* 1 * 32767 / 1700 */
    CAL_OK(&asym100, 1901, 0);
    CAL_OK(&asym100, 1900, 0);
    CAL_OK(&asym100, 1899, -23);   /* 1 * 32767 / 1400 */
    CAL_OK(&sym100, 2147, 0);
    CAL_OK(&sym100, 2148, 0);
    CAL_OK(&sym100, 2149, 16);     /* 1 * 32767 / 1947 */
    CAL_OK(&sym100, 1949, 0);
    CAL_OK(&sym100, 1948, 0);
    CAL_OK(&sym100, 1947, -16);
}

static void test_deadzone_validity_boundary(void)
{
    /* Narrow side is 200 above center: deadzone 199 is the largest valid one. */
    const struct sp3_axis_calibration valid = { 500, 2000, 2200, 199 };
    const struct sp3_axis_calibration at_up = { 500, 2000, 2200, 200 };
    const struct sp3_axis_calibration above_up = { 500, 2000, 2200, 201 };
    /* Narrow side is 1500 below center on a wide upper side. */
    const struct sp3_axis_calibration at_down = { 500, 2000, 3800, 1500 };
    const struct sp3_axis_calibration swapped = { 1000, 2000, 4095, 1000 };
    const struct sp3_axis_calibration wide_up_ok = { 1000, 2000, 4095, 999 };
    int16_t out = SENTINEL;

    EXPECT_TRUE(sp3_axis_calibrate(&valid, 2200, &out) == SP3_CAL_OK);
    EXPECT_TRUE(out == 32767);
    CAL_ERR(&at_up, 2100, SP3_CAL_ERROR_DEADZONE);
    CAL_ERR(&above_up, 2100, SP3_CAL_ERROR_DEADZONE);
    CAL_ERR(&at_down, 2100, SP3_CAL_ERROR_DEADZONE);
    CAL_ERR(&swapped, 2100, SP3_CAL_ERROR_DEADZONE);
    out = SENTINEL;
    EXPECT_TRUE(sp3_axis_calibrate(&wide_up_ok, 4095, &out) == SP3_CAL_OK);
    EXPECT_TRUE(out == 32767);
}

static void test_invalid_calibrations(void)
{
    const struct sp3_axis_calibration min_eq_center = { 500, 500, 3800, 0 };
    const struct sp3_axis_calibration center_eq_max = { 500, 3800, 3800, 0 };
    const struct sp3_axis_calibration min_above_center = { 2500, 2000, 3800, 0 };
    const struct sp3_axis_calibration center_above_max = { 500, 3900, 3800, 0 };
    const struct sp3_axis_calibration min_too_big = { 0x1000, 0x1001, 0x1002, 0 };
    const struct sp3_axis_calibration center_too_big = { 500, 0x1000, 0x1001, 0 };
    const struct sp3_axis_calibration max_too_big = { 500, 2000, 0x1000, 0 };
    const struct sp3_axis_calibration max_far_too_big = { 500, 2000, 0xffffu, 0 };

    CAL_ERR(&min_eq_center, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&center_eq_max, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&min_above_center, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&center_above_max, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&min_too_big, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&center_too_big, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&max_too_big, 2000, SP3_CAL_ERROR_RANGE);
    CAL_ERR(&max_far_too_big, 2000, SP3_CAL_ERROR_RANGE);
}

static void test_sample_domain_and_pointers(void)
{
    int16_t out = SENTINEL;

    /* Samples above 0x0fff are a domain error, not saturation. */
    CAL_ERR(&sym0, 0x1000, SP3_CAL_ERROR_SAMPLE);
    CAL_ERR(&sym100, 0x1000, SP3_CAL_ERROR_SAMPLE);
    CAL_ERR(&asym0, 0x1000, SP3_CAL_ERROR_SAMPLE);
    CAL_ERR(&asym0, 0xffffu, SP3_CAL_ERROR_SAMPLE);

    /* A bad calibration is reported before a bad sample. */
    {
        const struct sp3_axis_calibration bad = { 500, 500, 3800, 0 };
        CAL_ERR(&bad, 0x1000, SP3_CAL_ERROR_RANGE);
    }

    /* NULL cal: status only for the pointer, *out untouched. */
    EXPECT_TRUE(sp3_axis_calibrate(NULL, 2000, &out) == SP3_CAL_ERROR_ARGUMENT);
    EXPECT_TRUE(out == SENTINEL);
    /* NULL out: only the status code can be checked; nothing is dereferenced. */
    EXPECT_TRUE(sp3_axis_calibrate(&asym0, 2000, NULL) == SP3_CAL_ERROR_ARGUMENT);
    EXPECT_TRUE(sp3_axis_calibrate(NULL, 2000, NULL) == SP3_CAL_ERROR_ARGUMENT);
}

static void check_sweep(const struct sp3_axis_calibration *cal)
{
    int16_t previous = -32767;
    unsigned int raw;

    for (raw = 0; raw <= SP3_AXIS_RAW_MAX; raw++) {
        int16_t out = SENTINEL;

        if (sp3_axis_calibrate(cal, (uint16_t)raw, &out) != SP3_CAL_OK) {
            fprintf(stderr, "sweep: raw %u rejected\n", raw);
            failures++;
            return;
        }
        if (out < -32767 || out < previous) { /* int16_t cannot exceed 32767 */
            fprintf(stderr, "sweep: raw %u out %d (previous %d)\n", raw, (int)out,
                    (int)previous);
            failures++;
            return;
        }
        previous = out;
    }
}

static void test_sweeps_are_bounded_and_monotonic(void)
{
    const struct sp3_axis_calibration extra[] = {
        { 0, 1, 4095, 0 },
        { 0, 2048, 4095, 2046 },
        { 100, 3000, 3100, 50 },
        { 1000, 1100, 4000, 99 }
    };
    size_t i;

    check_sweep(&asym0);
    check_sweep(&asym100);
    check_sweep(&sym0);
    check_sweep(&sym100);
    for (i = 0; i < sizeof(extra) / sizeof(extra[0]); i++) {
        check_sweep(&extra[i]);
    }
}

/* ---- fixture loader -------------------------------------------------- */

static int parse_ok(const char *text, struct fixture_report *reports, size_t *count)
{
    char error[128];

    return fixture_parse("buffer", text, reports, 8, count, error, sizeof(error));
}

static int parse_error_is(const char *text, const char *expected_prefix)
{
    struct fixture_report reports[8];
    size_t count = 99;
    char error[128] = "";
    const int rc = fixture_parse("buffer", text, reports, 8, &count, error, sizeof(error));

    return rc == -1 && count == 99 &&
           strncmp(error, expected_prefix, strlen(expected_prefix)) == 0;
}

static void test_loader_accepts_the_documented_format(void)
{
    struct fixture_report reports[8];
    size_t count = 0;

    EXPECT_TRUE(parse_ok("30 01 FF aB\n", reports, &count) == 0);
    EXPECT_TRUE(count == 1 && reports[0].length == 4);
    EXPECT_TRUE(reports[0].data[0] == 0x30 && reports[0].data[2] == 0xff &&
                reports[0].data[3] == 0xab);

    count = 0;
    EXPECT_TRUE(parse_ok("# c\n\n  \t\n30 00 # tail\r\n\t31\t02\n", reports, &count) == 0);
    EXPECT_TRUE(count == 2 && reports[0].length == 2 && reports[1].length == 2);
    EXPECT_TRUE(reports[1].data[0] == 0x31 && reports[1].data[1] == 0x02);

    count = 0;
    EXPECT_TRUE(parse_ok("00", reports, &count) == 0);   /* no trailing newline */
    EXPECT_TRUE(count == 1 && reports[0].data[0] == 0x00);
}

static void test_loader_rejects_bad_input_with_a_line_number(void)
{
    char line[FIXTURE_MAX_LINE + 64];
    char report[FIXTURE_MAX_REPORT * 3 + 8];
    size_t i;
    struct fixture_report two[1];
    size_t count = 99;
    char error[128] = "";

    EXPECT_TRUE(parse_error_is("30 0\n", "buffer:1:"));        /* odd digit count */
    EXPECT_TRUE(parse_error_is("30\n0x31\n", "buffer:2:"));    /* 0x prefix */
    EXPECT_TRUE(parse_error_is("30 zz\n", "buffer:1:"));       /* non-hex */
    EXPECT_TRUE(parse_error_is("# ok\n30 300\n", "buffer:2:")); /* three digits */
    EXPECT_TRUE(parse_error_is("", "buffer:"));                /* empty */
    EXPECT_TRUE(parse_error_is("# only a comment\n\n", "buffer:")); /* no reports */

    memset(line, ' ', sizeof(line));
    line[FIXTURE_MAX_LINE + 1] = '\0';
    EXPECT_TRUE(parse_error_is(line, "buffer:1:"));            /* line too long */

    for (i = 0; i < FIXTURE_MAX_REPORT + 1; i++) {
        memcpy(&report[i * 3], "aa ", 3);
    }
    report[(FIXTURE_MAX_REPORT + 1) * 3] = '\0';
    EXPECT_TRUE(parse_error_is(report, "buffer:1:"));          /* report too long */

    /* The longest accepted report is exactly 64 bytes. */
    report[FIXTURE_MAX_REPORT * 3] = '\0';
    {
        struct fixture_report reports[8];
        EXPECT_TRUE(parse_ok(report, reports, &count) == 0);
        EXPECT_TRUE(count == 1 && reports[0].length == FIXTURE_MAX_REPORT);
    }

    count = 99;
    EXPECT_TRUE(fixture_parse("buffer", "30\n31\n", two, 1, &count, error,
                              sizeof(error)) == -1);
    EXPECT_TRUE(count == 99 && strncmp(error, "buffer:2:", 9) == 0);  /* capacity */
}

static void test_synthetic_fixture_file_parses_with_the_input_parser(void)
{
    struct fixture_report reports[8];
    struct sp3_input_state state;
    size_t count = 0;
    char error[160] = "";

    if (fixture_load_file("tests/fixtures/synthetic-input-reports.hex", reports, 8,
                          &count, error, sizeof(error)) != 0) {
        fprintf(stderr, "%s\n", error);
        failures++;
        return;
    }
    EXPECT_TRUE(count == 3);
    if (count != 3) {
        return;
    }

    EXPECT_TRUE(sp3_parse_input_report(reports[0].data, reports[0].length, &state) == SP3_OK);
    EXPECT_TRUE(state.left_x == 0x800u && state.left_y == 0x800u);
    EXPECT_TRUE(state.right_x == 0x800u && state.right_y == 0x800u && state.buttons == 0);

    EXPECT_TRUE(sp3_parse_input_report(reports[1].data, reports[1].length, &state) == SP3_OK);
    EXPECT_TRUE(state.buttons == (SP3_BUTTON_B | SP3_BUTTON_A));
    EXPECT_TRUE(state.left_x == 0x800u && state.right_y == 0x800u);

    EXPECT_TRUE(sp3_parse_input_report(reports[2].data, reports[2].length, &state) == SP3_OK);
    EXPECT_TRUE(state.left_x == 0xfffu && state.left_y == 0xfffu);
    EXPECT_TRUE(state.right_x == 0 && state.right_y == 0 && state.buttons == SP3_BUTTON_UP);
}

static void test_missing_fixture_file_is_an_error(void)
{
    struct fixture_report reports[2];
    size_t count = 99;
    char error[160] = "";

    EXPECT_TRUE(fixture_load_file("tests/fixtures/does-not-exist.hex", reports, 2,
                                  &count, error, sizeof(error)) == -1);
    EXPECT_TRUE(count == 99 && strstr(error, "cannot open") != NULL);
}

int main(void)
{
    test_fixed_points_and_saturation();
    test_deadzone_edges();
    test_deadzone_validity_boundary();
    test_invalid_calibrations();
    test_sample_domain_and_pointers();
    test_sweeps_are_bounded_and_monotonic();
    test_loader_accepts_the_documented_format();
    test_loader_rejects_bad_input_with_a_line_number();
    test_synthetic_fixture_file_parses_with_the_input_parser();
    test_missing_fixture_file_is_an_error();

    if (failures != 0) {
        fprintf(stderr, "%d test expectation(s) failed\n", failures);
        return 1;
    }

    puts("switchpro3 calibration tests: ok");
    return 0;
}
