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
step "shell syntax" bash -n tools/check.sh
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
    step "ShellCheck" shellcheck -S warning tools/check.sh
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

