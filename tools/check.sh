#!/usr/bin/env bash
# Runs locally what CI runs. A skipped check is reported and is not a pass.
set -u
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 2

since=""
while [ $# -gt 0 ]; do
    case "$1" in
        --since) since=${2:-}; shift ;;
        -h|--help)
            printf 'usage: tools/check.sh [--since REF]\n'
            exit 0
            ;;
        *) printf 'unknown option: %s\n' "$1" >&2; exit 2 ;;
    esac
    shift
done

fails=0
warns=0
skips=0
log=$(mktemp)
trap 'rm -f "$log"' EXIT

ok() { printf 'ok    %s\n' "$1"; }
bad() {
    printf 'FAIL  %s\n' "$1"
    fails=$((fails + 1))
    [ -s "$log" ] && sed 's/^/        /' "$log" | tail -30
}
skip() { printf 'skip  %s (%s)\n' "$1" "$2"; skips=$((skips + 1)); }
warn() { printf 'warn  %s\n' "$1"; warns=$((warns + 1)); }
step() {
    local description=$1
    shift
    : >"$log"
    if "$@" >"$log" 2>&1; then ok "$description"; else bad "$description"; fi
}

step "portable C build and unit tests" make test
step "GCC static analyzer" make test BUILD_DIR=build-analyzer CFLAGS='-O2 -fanalyzer'
# LeakSanitizer needs ptrace and does not work in every sandbox. Probe it instead of
# assuming: a clean program must pass and a leaking one must fail with it enabled.
# SP3_LEAK_SANITIZER=off forces it off (for reproducing a restricted environment).
leaks=off
if [ "${SP3_LEAK_SANITIZER:-auto}" != off ]; then
    probe=$(mktemp -d)
    printf 'int main(void){return 0;}\n' >"$probe/clean.c"
    printf '#include <stdlib.h>\nint main(void){volatile char *p=malloc(16);p[0]=1;p=0;return 0;}\n' >"$probe/leak.c"
    if cc -fsanitize=address -o "$probe/clean" "$probe/clean.c" 2>/dev/null &&
       cc -O0 -fsanitize=address -o "$probe/leak" "$probe/leak.c" 2>/dev/null &&
       ASAN_OPTIONS=detect_leaks=1 "$probe/clean" >/dev/null 2>&1 &&
       ! ASAN_OPTIONS=detect_leaks=1 "$probe/leak" >/dev/null 2>&1; then
        leaks=on
    fi
    rm -rf "$probe"
fi
if [ "$leaks" = on ]; then
    step "ASan/UBSan unit tests (LeakSanitizer active)" \
        env ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
        make test BUILD_DIR=build-sanitize \
        CFLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
else
    step "ASan/UBSan unit tests (leak detection OFF)" \
        env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
        make test BUILD_DIR=build-sanitize \
        CFLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    skip "LeakSanitizer" "unavailable or disabled here (ptrace restriction); sanitizers ran with detect_leaks=0"
fi
step "parser mutation check" tools/mutation-check.sh
step "calibration mutation check" tools/mutation-check-calibration.sh
step "shell syntax" bash -c 'for f in tools/*.sh; do bash -n "$f" || exit 1; done'
step "MIT SPDX headers" bash -c '
    missing=0
    while IFS= read -r file; do
        grep -q "SPDX-License-Identifier: MIT" "$file" || {
            echo "$file: missing MIT SPDX identifier"
            missing=1
        }
    done < <(find include src tests -type f \( -name "*.c" -o -name "*.h" \) -print)
    exit "$missing"
'
step "required project documentation" bash -c '
    for file in README.md CONTRIBUTING.md DEVLOG.md LICENSE docs/architecture.md docs/hardware-checklist.md; do
        [ -s "$file" ] || { echo "missing or empty: $file"; exit 1; }
    done
'

if command -v shellcheck >/dev/null 2>&1; then
    step "ShellCheck" shellcheck -S warning tools/*.sh
else
    skip "ShellCheck" "not installed: CI must run it once added as a blocking job"
fi

[ -n "$since" ] || since=$(git describe --tags --abbrev=0 2>/dev/null || true)
if [ -n "$since" ] && git rev-parse --verify -q "$since^{commit}" >/dev/null; then
    changed=$(git diff --name-only "$since"...HEAD 2>/dev/null)
    if printf '%s\n' "$changed" | grep -Eq '^(src/|include/|adapters/)' &&
       ! printf '%s\n' "$changed" | grep -q '^DEVLOG.md$'; then
        warn "behavior changed since $since but DEVLOG.md did not"
    fi
    if printf '%s\n' "$changed" | grep -Eq '^(src/|include/|adapters/)' &&
       ! printf '%s\n' "$changed" | grep -Eq '^(README.md|docs/)'; then
        warn "behavior changed since $since but README/docs did not"
    fi
else
    skip "documentation hints" "no tag or --since REF to compare with"
fi

printf '\n%d failed, %d warning(s), %d skipped\n' "$fails" "$warns" "$skips"
exit "$fails"

