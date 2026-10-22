#!/usr/bin/env bash
# Host-only mutation check for src/calibration.c. Each mutant is applied to a
# scratch copy; the calibration tests must fail for every one. Not part of
# tools/check.sh.
set -u
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 2

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc=${CC:-cc}
survivors=0
total=0

# name|sed expression applied to a copy of src/calibration.c
mutants=(
    # An exclusive edge (<) is an equivalent mutant: the rescale makes distance == deadzone map to 0 anyway.
    'edge-comparison-one-too-wide|s/distance <= (int32_t)cal->deadzone/distance <= (int32_t)cal->deadzone + 1/'
    'divisor-ignores-deadzone|s/(span - (int32_t)cal->deadzone)/span/'
    'numerator-ignores-deadzone|s/((distance - (int32_t)cal->deadzone) \* SP3_AXIS_FULL_SCALE)/(distance * SP3_AXIS_FULL_SCALE)/'
    'negative-side-uses-positive-span|s/span = (int32_t)cal->center - (int32_t)cal->min;/span = (int32_t)cal->max - (int32_t)cal->center;/'
    'missing-saturation|s/if (value > SP3_AXIS_FULL_SCALE)/if (0)/'
    'sign-dropped|s/(int16_t)(sign \* value)/(int16_t)value/'
    'deadzone-validation-up-too-lax|s/cal->deadzone >= cal->max - cal->center/cal->deadzone > cal->max - cal->center/'
    'deadzone-validation-down-too-lax|s/cal->deadzone >= cal->center - cal->min/cal->deadzone > cal->center - cal->min/'
    'min-center-validation-too-lax|s/cal->min >= cal->center/cal->min > cal->center/'
    'center-max-validation-too-lax|s/cal->center >= cal->max/cal->center > cal->max/'
    'max-domain-not-checked|s/cal->max > SP3_AXIS_RAW_MAX ||/0 ||/'
    'sample-domain-off-by-one|s/raw > SP3_AXIS_RAW_MAX/raw > SP3_AXIS_RAW_MAX + 1/'
    'write-before-validation|s/    if (cal->min > SP3_AXIS_RAW_MAX/    *out = 1;\n    if (cal->min > SP3_AXIS_RAW_MAX/'
    'null-out-not-checked|s/cal == NULL || out == NULL/cal == NULL/'
)

for entry in "${mutants[@]}"; do
    name=${entry%%|*}
    expr=${entry#*|}
    total=$((total + 1))
    rm -rf "${work:?}/m"
    mkdir "$work/m"
    cp -r include src tests "$work/m/"
    sed -i "$expr" "$work/m/src/calibration.c"
    if cmp -s src/calibration.c "$work/m/src/calibration.c"; then
        printf 'ERROR %s: mutation did not apply\n' "$name"
        exit 2
    fi
    if ! "$cc" -std=c11 -I"$work/m/include" -O1 -Itests \
        "$work/m/tests/test_calibration.c" "$work/m/src/calibration.c" \
        "$work/m/src/hid.c" -o "$work/m/t" 2>"$work/m/log"; then
        printf 'ERROR %s: mutant does not compile\n' "$name"
        exit 2
    fi
    if { (cd "$work/m" && ./t >/dev/null 2>&1); } 2>/dev/null; then
        printf 'SURVIVED %s\n' "$name"
        survivors=$((survivors + 1))
    else
        printf 'killed   %s\n' "$name"
    fi
done

printf '%d mutants, %d survived\n' "$total" "$survivors"
[ "$survivors" -eq 0 ]
