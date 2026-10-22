/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "switchpro3/hid.h"

#include <string.h>

enum {
    SP3_COMMON_INPUT_SIZE = 12
};

static uint16_t unpack_stick_x(const uint8_t packed[3])
{
    return (uint16_t)packed[0] | ((uint16_t)(packed[1] & 0x0fu) << 8);
}

static uint16_t unpack_stick_y(const uint8_t packed[3])
{
    return ((uint16_t)packed[1] >> 4) | ((uint16_t)packed[2] << 4);
}

static uint32_t parse_buttons(const uint8_t right, const uint8_t shared,
                              const uint8_t left)
{
    uint32_t buttons = 0;

    if ((right & 0x01u) != 0) buttons |= SP3_BUTTON_Y;
    if ((right & 0x02u) != 0) buttons |= SP3_BUTTON_X;
    if ((right & 0x04u) != 0) buttons |= SP3_BUTTON_B;
    if ((right & 0x08u) != 0) buttons |= SP3_BUTTON_A;
    if ((right & 0x40u) != 0) buttons |= SP3_BUTTON_R;
    if ((right & 0x80u) != 0) buttons |= SP3_BUTTON_ZR;

    if ((shared & 0x01u) != 0) buttons |= SP3_BUTTON_MINUS;
    if ((shared & 0x02u) != 0) buttons |= SP3_BUTTON_PLUS;
    if ((shared & 0x04u) != 0) buttons |= SP3_BUTTON_R_STICK;
    if ((shared & 0x08u) != 0) buttons |= SP3_BUTTON_L_STICK;
    if ((shared & 0x10u) != 0) buttons |= SP3_BUTTON_HOME;
    if ((shared & 0x20u) != 0) buttons |= SP3_BUTTON_CAPTURE;

    if ((left & 0x01u) != 0) buttons |= SP3_BUTTON_DOWN;
    if ((left & 0x02u) != 0) buttons |= SP3_BUTTON_UP;
    if ((left & 0x04u) != 0) buttons |= SP3_BUTTON_RIGHT;
    if ((left & 0x08u) != 0) buttons |= SP3_BUTTON_LEFT;
    if ((left & 0x40u) != 0) buttons |= SP3_BUTTON_L;
    if ((left & 0x80u) != 0) buttons |= SP3_BUTTON_ZL;

    return buttons;
}

enum sp3_status sp3_parse_input_report(const uint8_t *report,
                                       const size_t report_size,
                                       struct sp3_input_state *state)
{
    if (report == NULL || state == NULL) {
        return SP3_ERROR_ARGUMENT;
    }

    if (report_size == 0) {
        return SP3_ERROR_SHORT_REPORT;
    }

    if (report[0] != SP3_REPORT_SUBCOMMAND_REPLY &&
        report[0] != SP3_REPORT_FULL_INPUT &&
        report[0] != SP3_REPORT_MCU_INPUT) {
        return SP3_ERROR_UNSUPPORTED_REPORT;
    }

    if (report_size < SP3_COMMON_INPUT_SIZE) {
        return SP3_ERROR_SHORT_REPORT;
    }

    memset(state, 0, sizeof(*state));
    state->report_id = report[0];
    state->timer = report[1];
    state->battery_and_connection = report[2];
    state->buttons = parse_buttons(report[3], report[4], report[5]);
    state->left_x = unpack_stick_x(&report[6]);
    state->left_y = unpack_stick_y(&report[6]);
    state->right_x = unpack_stick_x(&report[9]);
    state->right_y = unpack_stick_y(&report[9]);

    return SP3_OK;
}

