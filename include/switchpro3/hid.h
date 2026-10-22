/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SWITCHPRO3_HID_H
#define SWITCHPRO3_HID_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SP3_NINTENDO_VENDOR_ID 0x057eu
#define SP3_SWITCH_PRO_PRODUCT_ID 0x2009u

#define SP3_REPORT_SUBCOMMAND_REPLY 0x21u
#define SP3_REPORT_FULL_INPUT 0x30u
#define SP3_REPORT_MCU_INPUT 0x31u
#define SP3_REPORT_SIMPLE_INPUT 0x3fu

enum sp3_status {
    SP3_OK = 0,
    SP3_ERROR_ARGUMENT = -1,
    SP3_ERROR_SHORT_REPORT = -2,
    SP3_ERROR_UNSUPPORTED_REPORT = -3
};

enum sp3_button {
    SP3_BUTTON_Y       = UINT32_C(1) << 0,
    SP3_BUTTON_X       = UINT32_C(1) << 1,
    SP3_BUTTON_B       = UINT32_C(1) << 2,
    SP3_BUTTON_A       = UINT32_C(1) << 3,
    SP3_BUTTON_R       = UINT32_C(1) << 4,
    SP3_BUTTON_ZR      = UINT32_C(1) << 5,
    SP3_BUTTON_MINUS   = UINT32_C(1) << 6,
    SP3_BUTTON_PLUS    = UINT32_C(1) << 7,
    SP3_BUTTON_R_STICK = UINT32_C(1) << 8,
    SP3_BUTTON_L_STICK = UINT32_C(1) << 9,
    SP3_BUTTON_HOME    = UINT32_C(1) << 10,
    SP3_BUTTON_CAPTURE = UINT32_C(1) << 11,
    SP3_BUTTON_DOWN    = UINT32_C(1) << 12,
    SP3_BUTTON_UP      = UINT32_C(1) << 13,
    SP3_BUTTON_RIGHT   = UINT32_C(1) << 14,
    SP3_BUTTON_LEFT    = UINT32_C(1) << 15,
    SP3_BUTTON_L       = UINT32_C(1) << 16,
    SP3_BUTTON_ZL      = UINT32_C(1) << 17
};

struct sp3_input_state {
    uint32_t buttons;
    uint16_t left_x;
    uint16_t left_y;
    uint16_t right_x;
    uint16_t right_y;
    uint8_t report_id;
    uint8_t timer;
    uint8_t battery_and_connection;
};

/*
 * Parse the common 12-byte input prefix used by report IDs 0x21, 0x30 and
 * 0x31. Stick values remain raw 12-bit samples so calibration can be applied
 * by a separate policy layer.
 */
enum sp3_status sp3_parse_input_report(
    const uint8_t *report,
    size_t report_size,
    struct sp3_input_state *state);

#ifdef __cplusplus
}
#endif

#endif

