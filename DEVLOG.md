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

### Check entry point runs sanitizers, mutation harnesses and ShellCheck on every script

- **What changed:** `tools/check.sh` now also builds and runs both unit suites
  with `-fsanitize=address,undefined`, runs both mutation harnesses
  (`tools/mutation-check.sh`, `tools/mutation-check-calibration.sh`), checks the
  syntax of every `tools/*.sh`, and runs ShellCheck on every `tools/*.sh`
  instead of `check.sh` alone.
- **Why:** these checks were manual evidence so far; the earlier red CI run and
  the review of the sanitizer claim showed that manual evidence is easy to
  mis-state. CI runs `tools/check.sh`, so it now runs them too.
- **Evidence (host):** `tools/check.sh` 0 failed, 0 warnings, 1 skipped
  (documentation hints without a tag); ShellCheck 0.11.0 now runs locally and
  passes on every `tools/*.sh`. The sanitizer binary is instrumented (`nm`
  shows ASan symbols). In a scratch copy with one test call removed, the
  parser mutation step failed, so the wiring detects a weakened suite. CI
  has not run this change yet.
- **Leak detection is probed, not assumed:** LeakSanitizer needs ptrace and
  fails inside some sandboxes (a reviewer environment reported `LeakSanitizer
  has encountered a fatal error`). `tools/check.sh` now compiles two tiny probes
  (one clean, one leaking) and enables leak detection only if the clean one
  passes and the leaking one fails. Otherwise the sanitizer step is labelled
  `leak detection OFF`, runs with `ASAN_OPTIONS=detect_leaks=0`, and a separate
  `skip LeakSanitizer` line appears, so "sanitizers passed with leaks off" can no
  longer be read as "LeakSanitizer passed". `SP3_LEAK_SANITIZER=off` reproduces
  the restricted case anywhere. Both modes pass locally (active on this machine;
  forced off via the variable). The restricted environment itself has not been
  re-run by the author.
- **Open questions:** the mutation steps take a few seconds locally; whether the
  CI runner enables LeakSanitizer is only known after the first run.

### M1 Part A: axis calibration, fixture loader and mutation harness (SP3-CL-02)

- **What changed:** `sp3_axis_calibrate()` maps a raw 12-bit sample to a signed
  `int16_t` in `[-32767, 32767]` with a raw-sample dead zone applied around the
  center and rescaled so the output is continuous at the edge. It validates
  `min < center < max <= 0x0fff`, a dead zone smaller than the travel on each
  side, and samples up to `0x0fff`; every error leaves `*out` unchanged. A
  test-only fixture loader (`tests/fixture_loader.h`) reads hex report files
  and rejects malformed input with a line number; the first fixture,
  `tests/fixtures/synthetic-input-reports.hex`, is synthetic. The reply
  decoders (Part B) are not started.
- **Why:** M1 needs calibration and malformed-input coverage proven on the host
  before any real report is captured. The SPI calibration layout is a protocol
  fact still pending in `SP3-CX-02`, so this code is layout-independent.
- **Evidence (synthetic host fixtures only):** `tools/check.sh` 0 failed, 0
  warnings, 2 skipped (ShellCheck not installed, hints without a tag).
  `make test` runs the HID and calibration suites. The calibration suite covers
  fixed points, saturation, dead-zone edges, validity boundaries and a sweep of
  all 4096 samples (bounded and monotonic) for eight calibrations. Both suites
  also passed with `-fsanitize=address,undefined` on GCC 16.2.1 and the
  calibration suite on clang (plain and sanitized); those runs are manual and
  not part of `tools/check.sh`. `tools/mutation-check-calibration.sh` kills 14
  of 14 mutants. An exclusive dead-zone edge (`<` instead of `<=`) is an
  equivalent mutant (the rescale maps `distance == deadzone` to 0 anyway), so
  the harness uses a one-step-wider edge instead.
- **Open questions:** no real controller report or real calibration data was
  used; the dead-zone semantics are a design choice, not measured behaviour.
  CI with the runner's compiler has not run on this change yet.

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
  state reset, write before validation) each made the suite fail; they were
  run by hand and are now reproducible with `tools/mutation-check.sh`, which
  also covers two unassigned-bit leaks (9 mutants, 0 survived). The same suite
  built with `-fsanitize=address,undefined` passed on GCC 16.2.1; that run is
  not part of `tools/check.sh`. The test proves only that trailing bytes do not
  change the result; it does not instrument memory reads.
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

