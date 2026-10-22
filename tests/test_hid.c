/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "switchpro3/hid.h"

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

static void pack_stick(uint8_t packed[3], const uint16_t x, const uint16_t y)
{
    packed[0] = (uint8_t)(x & 0xffu);
    packed[1] = (uint8_t)(((x >> 8) & 0x0fu) | ((y & 0x0fu) << 4));
    packed[2] = (uint8_t)((y >> 4) & 0xffu);
}

static void test_full_input_report(void)
{
    uint8_t report[12] = {
        SP3_REPORT_FULL_INPUT, 0x42u, 0x91u,
        0x4du, 0x31u, 0xc6u,
        0, 0, 0, 0, 0, 0
    };
    struct sp3_input_state state;
    const uint32_t expected_buttons =
        SP3_BUTTON_Y | SP3_BUTTON_B | SP3_BUTTON_A | SP3_BUTTON_R |
        SP3_BUTTON_MINUS | SP3_BUTTON_HOME | SP3_BUTTON_CAPTURE |
        SP3_BUTTON_UP | SP3_BUTTON_RIGHT | SP3_BUTTON_L | SP3_BUTTON_ZL;

    pack_stick(&report[6], 0x123u, 0xabcu);
    pack_stick(&report[9], 0xfffu, 0x000u);

    EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
    EXPECT_TRUE(state.report_id == SP3_REPORT_FULL_INPUT);
    EXPECT_TRUE(state.timer == 0x42u);
    EXPECT_TRUE(state.battery_and_connection == 0x91u);
    EXPECT_TRUE(state.buttons == expected_buttons);
    EXPECT_TRUE(state.left_x == 0x123u);
    EXPECT_TRUE(state.left_y == 0xabcu);
    EXPECT_TRUE(state.right_x == 0xfffu);
    EXPECT_TRUE(state.right_y == 0x000u);
}

static void test_common_prefix_on_subcommand_reply(void)
{
    uint8_t report[12] = { SP3_REPORT_SUBCOMMAND_REPLY };
    struct sp3_input_state state;

    pack_stick(&report[6], 0x800u, 0x800u);
    pack_stick(&report[9], 0x800u, 0x800u);

    EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
    EXPECT_TRUE(state.left_x == 0x800u);
    EXPECT_TRUE(state.right_y == 0x800u);
}

static void test_rejected_inputs_do_not_modify_output(void)
{
    const uint8_t simple_report[12] = { SP3_REPORT_SIMPLE_INPUT };
    const uint8_t short_report[1] = { SP3_REPORT_FULL_INPUT };
    struct sp3_input_state state;
    struct sp3_input_state original;

    memset(&state, 0xa5, sizeof(state));
    original = state;
    EXPECT_TRUE(sp3_parse_input_report(simple_report, sizeof(simple_report), &state) ==
                SP3_ERROR_UNSUPPORTED_REPORT);
    EXPECT_TRUE(memcmp(&state, &original, sizeof(state)) == 0);

    EXPECT_TRUE(sp3_parse_input_report(short_report, sizeof(short_report), &state) ==
                SP3_ERROR_SHORT_REPORT);
    EXPECT_TRUE(memcmp(&state, &original, sizeof(state)) == 0);
    EXPECT_TRUE(sp3_parse_input_report(NULL, 0, &state) == SP3_ERROR_ARGUMENT);
    EXPECT_TRUE(sp3_parse_input_report(short_report, sizeof(short_report), NULL) ==
                SP3_ERROR_ARGUMENT);
}

int main(void)
{
    test_full_input_report();
    test_common_prefix_on_subcommand_reply();
    test_rejected_inputs_do_not_modify_output();

    if (failures != 0) {
        fprintf(stderr, "%d test expectation(s) failed\n", failures);
        return 1;
    }

    puts("switchpro3 HID tests: ok");
    return 0;
}

