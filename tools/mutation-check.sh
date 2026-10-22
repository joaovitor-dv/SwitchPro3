#!/usr/bin/env bash
# Host-only mutation check for src/hid.c. Each mutant is applied to a scratch
# copy; the unit tests must fail for every one. Not part of tools/check.sh.
set -u
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 2

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc=${CC:-cc}
survivors=0
total=0

# name|sed expression applied to a copy of src/hid.c
mutants=(
    'wrong-ZL-mask|s/(left & 0x80u)/(left \& 0x40u)/'
    'length-limit-off-by-one|s/report_size < SP3_COMMON_INPUT_SIZE/report_size < SP3_COMMON_INPUT_SIZE - 1/'
    'wrong-0x31-handling|s/report\[0\] != SP3_REPORT_MCU_INPUT/report[0] != 0x31u + 1u/'
    'wrong-stick-y-shift|s/(uint16_t)packed\[2\] << 4/(uint16_t)packed[2] << 5/'
    'wrong-stick-x-mask|s/packed\[1\] & 0x0fu/packed[1] \& 0x1fu/'
    'missing-state-reset|s/memset(state, 0, sizeof(\*state));//'
    'shared-0x40-leaks-into-HOME|s/(shared & 0x10u)/(shared \& 0x50u)/'
    'left-0x20-leaks-into-L|s/(left & 0x40u) != 0) buttons |= SP3_BUTTON_L;/(left \& 0x60u) != 0) buttons |= SP3_BUTTON_L;/'
    'write-before-validation|s/    if (report_size == 0) {/    state->timer = 1;\n    if (report_size == 0) {/'
)

for entry in "${mutants[@]}"; do
    name=${entry%%|*}
    expr=${entry#*|}
    total=$((total + 1))
    rm -rf "${work:?}/m"
    mkdir "$work/m"
    cp -r include src tests "$work/m/"
    sed -i "$expr" "$work/m/src/hid.c"
    if cmp -s src/hid.c "$work/m/src/hid.c"; then
        printf 'ERROR %s: mutation did not apply\n' "$name"
        exit 2
    fi
    if ! "$cc" -std=c11 -Iinclude -I"$work/m/include" -O1 \
        "$work/m/tests/test_hid.c" "$work/m/src/hid.c" -o "$work/m/t" 2>"$work/m/log"; then
        printf 'ERROR %s: mutant does not compile\n' "$name"
        exit 2
    fi
    if "$work/m/t" >/dev/null 2>&1; then
        printf 'SURVIVED %s\n' "$name"
        survivors=$((survivors + 1))
    else
        printf 'killed   %s\n' "$name"
    fi
done

printf '%d mutants, %d survived\n' "$total" "$survivors"
[ "$survivors" -eq 0 ]
