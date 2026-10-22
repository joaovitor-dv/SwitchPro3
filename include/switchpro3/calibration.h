/* SPDX-License-Identifier: MIT */
#ifndef SWITCHPRO3_CALIBRATION_H
#define SWITCHPRO3_CALIBRATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SP3_AXIS_RAW_MAX 0x0fffu
#define SP3_AXIS_FULL_SCALE 32767

/* Per-axis calibration in raw 12-bit sample units. */
struct sp3_axis_calibration {
    uint16_t min;
    uint16_t center;
    uint16_t max;
    uint16_t deadzone;
};

enum sp3_cal_status {
    SP3_CAL_OK = 0,
    SP3_CAL_ERROR_ARGUMENT = -1,
    SP3_CAL_ERROR_RANGE = -2,
    SP3_CAL_ERROR_DEADZONE = -3,
    SP3_CAL_ERROR_SAMPLE = -4
};

/*
 * Map a raw sample to a signed output in [-32767, 32767] (-32768 is never
 * produced). The dead zone is measured in raw samples around `center`
 * (|raw - center| <= deadzone gives 0); the remaining travel is rescaled so
 * the output is continuous at the dead-zone edge and saturates at the ends.
 *
 * Checks run in this order and every error leaves *out unchanged:
 * NULL pointers, min < center < max <= 0x0fff, deadzone smaller than the
 * travel on each side, raw <= 0x0fff.
 */
enum sp3_cal_status sp3_axis_calibrate(
    const struct sp3_axis_calibration *cal,
    uint16_t raw,
    int16_t *out);

#ifdef __cplusplus
}
#endif

#endif
