# SwitchPro3 devlog

A running log of what we build, why, and what we measured. The
[README](README.md) is the user overview; this file records engineering
evidence and remaining uncertainty.

- **Newest first.** Every release tag gets an entry; unreleased work remains
  under *Unreleased*.
- **Honest by default.** Distinguish host tests, real-controller tests, PS3
  tests and reasoning.
- **Dates** use America/Sao_Paulo (UTC-3).

## Entry template

```markdown
### Short title

- **What changed:** ...
- **Why:** ...
- **Evidence:** ...
- **Open questions:** ...
```

---

## Unreleased

### Parser hardening: table-driven host coverage (SP3-CL-01)

- **What changed:** `tests/test_hid.c` now covers every defined button bit for
  report IDs `0x21`, `0x30` and `0x31`, rejects every other report ID, rejects
  every truncated length from 0 to 11 bytes, and checks that rejected input
  leaves the caller's state untouched. It also checks that trailing bytes are
  ignored and that stick values stay within 12 bits. No production code changed.
- **Why:** the parser will sit inside VSH, so malformed and short input must be
  proven harmless before any transport feeds it.
- **Evidence (synthetic host fixtures only):** `tools/check.sh` on CachyOS:
  0 failed, 0 warnings, 2 skipped (ShellCheck not installed, documentation
  hints without a tag). Seven manual mutations of `src/hid.c` (wrong ZL mask,
  length limit off by one, wrong `0x31` handling, wrong stick shifts, missing
  state reset, write before validation) each made the suite fail. The test
  proves only that trailing bytes do not change the result; it does not
  instrument memory reads.
- **Open questions:** no real controller report was used; the bit layout is
  still taken from the audited protocol facts, not from a captured trace.
  Report `0x3f` remains unsupported by design.

### Two-thread work partition

- **What changed:** the next phase is split into an exclusive Claude
  parser-hardening task and an exclusive Codex clean-room USB/PS3 research
  task under `docs/coordination/`.
- **Why:** implementation and protocol research can progress in parallel
  without concurrent edits, undocumented packet assumptions or accidental
  reuse of GPL/unlicensed source.
- **Evidence:** each task brief defines its write scope, acceptance criteria,
  stop conditions and handoff format. No hardware or remote action was
  performed for this partition.
- **Open questions:** the research handoff must be reviewed before USB or PS3
  transport code is authorized.

### UJM governance adapted to the independent repository

- **What changed:** SwitchPro3 now uses UJM's MIT license, SemVer 0.x policy,
  English Conventional Commits, same-commit documentation, a single local/CI
  check entry point, a hardware checklist and annotated release tags.
- **Why:** the project is independent from UJM's code and Claude coordination,
  but should retain the same auditable maintenance and release discipline.
- **Evidence:** `tools/check.sh` verifies the build, unit tests, license headers,
  required documentation and shell syntax. No remote action was performed.
- **Open questions:** CI cannot be observed until a remote exists and a push is
  explicitly authorized.

### Portable Nintendo HID parser foundation

- **What changed:** a C11 library parses the common `0x21`, `0x30` and `0x31`
  Nintendo input prefix into a neutral state with raw 12-bit stick values.
- **Why:** protocol parsing can be tested safely on CachyOS before introducing
  PS3 SDK, VSH, USB ownership or Bluetooth risks.
- **Evidence:** strict GCC build, unit tests, GCC static analyzer and ASan/UBSan
  passed on 2026-10-22. The existing-code audit pins all reference revisions.
  **Not tested with a real controller or PS3.**
- **Open questions:** `0x3f`, calibration, USB initialization, output reports,
  rumble, IMU, PS3 virtual-pad injection and Bluetooth remain unimplemented.

