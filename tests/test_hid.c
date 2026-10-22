/* SPDX-License-Identifier: MIT */
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

struct button_case {
    const char *name;
    size_t offset;
    uint8_t mask;
    uint32_t button;
};

static const struct button_case button_cases[] = {
    { "Y",       3, 0x01u, SP3_BUTTON_Y },
    { "X",       3, 0x02u, SP3_BUTTON_X },
    { "B",       3, 0x04u, SP3_BUTTON_B },
    { "A",       3, 0x08u, SP3_BUTTON_A },
    { "R",       3, 0x40u, SP3_BUTTON_R },
    { "ZR",      3, 0x80u, SP3_BUTTON_ZR },
    { "MINUS",   4, 0x01u, SP3_BUTTON_MINUS },
    { "PLUS",    4, 0x02u, SP3_BUTTON_PLUS },
    { "R_STICK", 4, 0x04u, SP3_BUTTON_R_STICK },
    { "L_STICK", 4, 0x08u, SP3_BUTTON_L_STICK },
    { "HOME",    4, 0x10u, SP3_BUTTON_HOME },
    { "CAPTURE", 4, 0x20u, SP3_BUTTON_CAPTURE },
    { "DOWN",    5, 0x01u, SP3_BUTTON_DOWN },
    { "UP",      5, 0x02u, SP3_BUTTON_UP },
    { "RIGHT",   5, 0x04u, SP3_BUTTON_RIGHT },
    { "LEFT",    5, 0x08u, SP3_BUTTON_LEFT },
    { "L",       5, 0x40u, SP3_BUTTON_L },
    { "ZL",      5, 0x80u, SP3_BUTTON_ZL }
};

static const uint8_t supported_ids[] = {
    SP3_REPORT_SUBCOMMAND_REPLY, SP3_REPORT_FULL_INPUT, SP3_REPORT_MCU_INPUT
};

static int is_supported_id(const unsigned int id)
{
    size_t i;

    for (i = 0; i < sizeof(supported_ids); i++) {
        if (supported_ids[i] == id) {
            return 1;
        }
    }
    return 0;
}

static void test_each_button_bit_maps_to_one_button(void)
{
    size_t i;
    size_t id;

    for (id = 0; id < sizeof(supported_ids); id++) {
        for (i = 0; i < sizeof(button_cases) / sizeof(button_cases[0]); i++) {
            uint8_t report[12] = { 0 };
            struct sp3_input_state state;

            report[0] = supported_ids[id];
            report[button_cases[i].offset] = button_cases[i].mask;
            if (sp3_parse_input_report(report, sizeof(report), &state) != SP3_OK ||
                state.buttons != button_cases[i].button) {
                fprintf(stderr, "button %s in report 0x%02x: got 0x%08x, want 0x%08x\n",
                        button_cases[i].name, supported_ids[id],
                        (unsigned int)state.buttons,
                        (unsigned int)button_cases[i].button);
                failures++;
            }
        }
    }
}

static void test_button_masks_are_distinct_and_complete(void)
{
    uint32_t seen = 0;
    size_t i;

    for (i = 0; i < sizeof(button_cases) / sizeof(button_cases[0]); i++) {
        EXPECT_TRUE((seen & button_cases[i].button) == 0);
        seen |= button_cases[i].button;
    }
    EXPECT_TRUE(seen == UINT32_C(0x3ffff));
}

static void test_unused_button_bits_are_ignored(void)
{
    /* Bits with no assigned button (0x10/0x20 of the right and left bytes,
     * 0xc0 of the shared byte) must not leak into the button mask. */
    uint8_t report[12] = { SP3_REPORT_FULL_INPUT, 0, 0, 0x30u, 0xc0u, 0x30u };
    struct sp3_input_state state;

    EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
    EXPECT_TRUE(state.buttons == 0);
}

static void test_all_supported_ids_parse_identically(void)
{
    struct sp3_input_state reference;
    size_t id;

    for (id = 0; id < sizeof(supported_ids); id++) {
        uint8_t report[12] = { 0, 0x07u, 0x80u, 0x08u, 0x02u, 0x40u };
        struct sp3_input_state state;

        report[0] = supported_ids[id];
        pack_stick(&report[6], 0x234u, 0x567u);
        pack_stick(&report[9], 0x89au, 0xbcdu);
        EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
        EXPECT_TRUE(state.report_id == supported_ids[id]);
        EXPECT_TRUE(state.timer == 0x07u);
        EXPECT_TRUE(state.battery_and_connection == 0x80u);
        EXPECT_TRUE(state.buttons == (SP3_BUTTON_A | SP3_BUTTON_PLUS | SP3_BUTTON_L));
        if (id == 0) {
            reference = state;
        } else {
            EXPECT_TRUE(state.buttons == reference.buttons);
            EXPECT_TRUE(state.left_x == reference.left_x && state.left_y == reference.left_y);
            EXPECT_TRUE(state.right_x == reference.right_x && state.right_y == reference.right_y);
        }
    }
}

static void test_every_report_id_other_than_the_three_is_rejected(void)
{
    unsigned int id;

    for (id = 0; id < 256u; id++) {
        uint8_t report[12] = { 0 };
        struct sp3_input_state state;
        struct sp3_input_state original;
        enum sp3_status status;

        report[0] = (uint8_t)id;
        memset(&state, 0x5a, sizeof(state));
        original = state;
        status = sp3_parse_input_report(report, sizeof(report), &state);
        if (is_supported_id(id)) {
            EXPECT_TRUE(status == SP3_OK);
        } else {
            EXPECT_TRUE(status == SP3_ERROR_UNSUPPORTED_REPORT);
            EXPECT_TRUE(memcmp(&state, &original, sizeof(state)) == 0);
        }
    }
}

static void test_every_truncated_length_is_rejected_unchanged(void)
{
    size_t id;

    for (id = 0; id < sizeof(supported_ids); id++) {
        uint8_t report[12];
        size_t length;

        memset(report, 0xff, sizeof(report));
        report[0] = supported_ids[id];
        for (length = 0; length < sizeof(report); length++) {
            struct sp3_input_state state;
            struct sp3_input_state original;

            memset(&state, 0xa5, sizeof(state));
            original = state;
            if (sp3_parse_input_report(report, length, &state) != SP3_ERROR_SHORT_REPORT ||
                memcmp(&state, &original, sizeof(state)) != 0) {
                fprintf(stderr, "report 0x%02x length %zu was not rejected cleanly\n",
                        supported_ids[id], length);
                failures++;
            }
        }
    }
}

static void test_unsupported_id_wins_over_truncation(void)
{
    const uint8_t report[1] = { SP3_REPORT_SIMPLE_INPUT };
    struct sp3_input_state state;

    EXPECT_TRUE(sp3_parse_input_report(report, 1, &state) == SP3_ERROR_UNSUPPORTED_REPORT);
    EXPECT_TRUE(sp3_parse_input_report(report, 0, &state) == SP3_ERROR_SHORT_REPORT);
}

static void test_trailing_bytes_are_accepted_and_ignored(void)
{
    uint8_t exact[12] = { SP3_REPORT_FULL_INPUT, 0x11u, 0x22u, 0x08u, 0x10u, 0x40u };
    uint8_t padded_zero[64] = { 0 };
    uint8_t padded_ones[64];
    struct sp3_input_state a, b, c;

    pack_stick(&exact[6], 0x321u, 0xfedu);
    pack_stick(&exact[9], 0x001u, 0x7ffu);
    memcpy(padded_zero, exact, sizeof(exact));
    memset(padded_ones, 0xff, sizeof(padded_ones));
    memcpy(padded_ones, exact, sizeof(exact));

    EXPECT_TRUE(sp3_parse_input_report(exact, sizeof(exact), &a) == SP3_OK);
    EXPECT_TRUE(sp3_parse_input_report(padded_zero, sizeof(padded_zero), &b) == SP3_OK);
    EXPECT_TRUE(sp3_parse_input_report(padded_ones, sizeof(padded_ones), &c) == SP3_OK);
    EXPECT_TRUE(memcmp(&a, &b, sizeof(a)) == 0);
    EXPECT_TRUE(memcmp(&a, &c, sizeof(a)) == 0);
}

static void test_stick_values_stay_in_twelve_bits(void)
{
    static const uint16_t samples[] = { 0x000u, 0x001u, 0x7ffu, 0x800u, 0xffeu, 0xfffu };
    size_t xi, yi;

    for (xi = 0; xi < sizeof(samples) / sizeof(samples[0]); xi++) {
        for (yi = 0; yi < sizeof(samples) / sizeof(samples[0]); yi++) {
            uint8_t report[12] = { SP3_REPORT_FULL_INPUT };
            struct sp3_input_state state;

            pack_stick(&report[6], samples[xi], samples[yi]);
            pack_stick(&report[9], samples[yi], samples[xi]);
            EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
            EXPECT_TRUE(state.left_x == samples[xi] && state.left_y == samples[yi]);
            EXPECT_TRUE(state.right_x == samples[yi] && state.right_y == samples[xi]);
            EXPECT_TRUE(state.left_x <= 0xfffu && state.left_y <= 0xfffu);
            EXPECT_TRUE(state.right_x <= 0xfffu && state.right_y <= 0xfffu);
        }
    }

    {
        uint8_t report[12];
        struct sp3_input_state state;

        memset(report, 0xff, sizeof(report));
        report[0] = SP3_REPORT_FULL_INPUT;
        EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
        EXPECT_TRUE(state.left_x == 0xfffu && state.left_y == 0xfffu);
        EXPECT_TRUE(state.right_x == 0xfffu && state.right_y == 0xfffu);
    }
}

static void test_parse_overwrites_every_field(void)
{
    uint8_t report[12] = { SP3_REPORT_FULL_INPUT };
    struct sp3_input_state state;

    memset(&state, 0xa5, sizeof(state));
    EXPECT_TRUE(sp3_parse_input_report(report, sizeof(report), &state) == SP3_OK);
    EXPECT_TRUE(state.buttons == 0);
    EXPECT_TRUE(state.left_x == 0 && state.left_y == 0);
    EXPECT_TRUE(state.right_x == 0 && state.right_y == 0);
    EXPECT_TRUE(state.timer == 0 && state.battery_and_connection == 0);
}

int main(void)
{
    test_full_input_report();
    test_common_prefix_on_subcommand_reply();
    test_rejected_inputs_do_not_modify_output();
    test_each_button_bit_maps_to_one_button();
    test_button_masks_are_distinct_and_complete();
    test_unused_button_bits_are_ignored();
    test_all_supported_ids_parse_identically();
    test_every_report_id_other_than_the_three_is_rejected();
    test_every_truncated_length_is_rejected_unchanged();
    test_unsupported_id_wins_over_truncation();
    test_trailing_bytes_are_accepted_and_ignored();
    test_stick_values_stay_in_twelve_bits();
    test_parse_overwrites_every_field();

    if (failures != 0) {
        fprintf(stderr, "%d test expectation(s) failed\n", failures);
        return 1;
    }

    puts("switchpro3 HID tests: ok");
    return 0;
}

