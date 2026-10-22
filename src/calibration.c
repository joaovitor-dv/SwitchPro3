/* SPDX-License-Identifier: MIT */
#include "switchpro3/calibration.h"

#include <stddef.h>

enum sp3_cal_status sp3_axis_calibrate(const struct sp3_axis_calibration *cal,
                                       const uint16_t raw, int16_t *out)
{
    int32_t offset;
    int32_t distance;
    int32_t span;
    int32_t sign;
    int32_t value;

    if (cal == NULL || out == NULL) {
        return SP3_CAL_ERROR_ARGUMENT;
    }

    if (cal->min > SP3_AXIS_RAW_MAX || cal->center > SP3_AXIS_RAW_MAX ||
        cal->max > SP3_AXIS_RAW_MAX || cal->min >= cal->center ||
        cal->center >= cal->max) {
        return SP3_CAL_ERROR_RANGE;
    }

    if (cal->deadzone >= cal->center - cal->min ||
        cal->deadzone >= cal->max - cal->center) {
        return SP3_CAL_ERROR_DEADZONE;
    }

    if (raw > SP3_AXIS_RAW_MAX) {
        return SP3_CAL_ERROR_SAMPLE;
    }

    offset = (int32_t)raw - (int32_t)cal->center;
    if (offset >= 0) {
        distance = offset;
        span = (int32_t)cal->max - (int32_t)cal->center;
        sign = 1;
    } else {
        distance = -offset;
        span = (int32_t)cal->center - (int32_t)cal->min;
        sign = -1;
    }

    if (distance <= (int32_t)cal->deadzone) {
        *out = 0;
        return SP3_CAL_OK;
    }

    value = ((distance - (int32_t)cal->deadzone) * SP3_AXIS_FULL_SCALE) /
            (span - (int32_t)cal->deadzone);
    if (value > SP3_AXIS_FULL_SCALE) {
        value = SP3_AXIS_FULL_SCALE;
    }

    *out = (int16_t)(sign * value);
    return SP3_CAL_OK;
}
