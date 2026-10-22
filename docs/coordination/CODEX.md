# Codex task — parser hardening and host evidence

## Assignment

- **Task ID:** `SP3-CX-01`
- **Owner:** Codex
- **Status:** ready
- **Dependency:** none
- **Hardware boundary:** host-only; no controller or PS3 operation

## Objective

Harden the existing portable parser and its CachyOS test evidence without
expanding into USB transport, PS3 APIs, output reports, rumble or Bluetooth.

## Exclusive write scope

- `include/switchpro3/hid.h`
- `src/hid.c`
- `tests/test_hid.c`
- `Makefile`, only if the test target needs a justified correction
- `tools/check.sh`, only if a check needs a justified correction
- `DEVLOG.md`, for evidence produced by this task
- this file, for the final handoff

Read access to the rest of the repository is unrestricted. Do not edit the
Claude-owned files listed in `docs/coordination/CLAUDE.md`.

## Work package

1. Add table-driven coverage for every currently defined button bit.
2. Cover supported report IDs `0x21`, `0x30` and `0x31` consistently.
3. Verify every truncated common report length from 0 through 11 bytes.
4. Verify rejected inputs leave the caller's output state unchanged.
5. Verify valid reports with trailing bytes are accepted without reading
   beyond the common prefix.
6. Keep raw stick values bounded to their 12-bit representation and cover
   minimum, midpoint and maximum values.
7. Run the repository check entry point and record exact host evidence.

## Acceptance criteria

- Public interfaces remain freestanding-friendly C11.
- No platform header or dynamic allocation enters the protocol core.
- Tests fail before a relevant regression and pass after the change.
- `tools/check.sh` passes; any skipped tool is reported as skipped, not passed.
- `DEVLOG.md` distinguishes synthetic host evidence from hardware evidence.
- One focused Conventional Commit is created locally. Do not push.

## Stop conditions

Stop and report instead of guessing if the work requires an undocumented
Nintendo packet layout, GPL/unlicensed source adaptation, a real controller,
PS3 SDK symbols or changes outside the exclusive write scope.

## Handoff

Append a short completion section here containing the commit hash, files
changed, commands run, results, unresolved risks and the next exact action.

