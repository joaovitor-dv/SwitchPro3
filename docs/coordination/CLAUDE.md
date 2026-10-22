# Claude task — parser hardening and host evidence

## Assignment

- **Task ID:** `SP3-CL-01`
- **Owner:** Claude
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
Codex-owned files listed in `docs/coordination/CODEX.md`.

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
- One focused Conventional Commit is created locally using exact-path staging.
  Do not include Codex research files and do not push.

## Stop conditions

Stop and report instead of guessing if the work requires an undocumented
Nintendo packet layout, GPL/unlicensed source adaptation, a real controller,
PS3 SDK symbols or changes outside the exclusive write scope.

## Handoff

Append a short completion section here containing the commit hash, files
changed, commands run, results, unresolved risks and the next exact action.


### 2026-10-22 completion — SP3-CL-01

- **Commit:** `cbacfc0` (`test: cover every button, report ID and truncated length in the parser`), local, not pushed.
- **Files changed:** `tests/test_hid.c`, `DEVLOG.md`. No production code change was needed.
- **Commands:** `make test`; `tools/check.sh` → 0 failed, 0 warnings, 2 skipped (ShellCheck not
  installed, documentation hints without a tag). Seven manual mutations of `src/hid.c` each failed the suite.
- **Coverage:** all 18 button bits across `0x21`/`0x30`/`0x31`; all 253 other report IDs rejected with state
  unchanged; every length 0–11 rejected with state unchanged; trailing bytes ignored; 12-bit stick range.
- **Unresolved risks:** synthetic fixtures only, no captured controller trace; "no read past the prefix" is
  shown by result equivalence under different trailing bytes, not by memory instrumentation (ASan/UBSan are not
  part of `tools/check.sh`); `0x3f` stays unsupported; the parser exposes no ack/subcommand fields.
- **Cross-review of SP3-CX-01:** done, findings in the Claude-side internal notes (citation paths, a possible
  second handshake, decoder for `0x81`/`0x21` ack fields). Nothing in `docs/research/` or `CODEX.md` was edited.
- **Next exact action:** Codex addresses the research findings; then open a host-only task for fixtures and
  the USB state machine plus a separate reply decoder.
