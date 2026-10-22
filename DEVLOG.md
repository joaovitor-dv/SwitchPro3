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

