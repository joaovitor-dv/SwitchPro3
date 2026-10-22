# Codex task — USB and PS3 clean-room research

## Assignment

- **Task ID:** `SP3-CX-01`
- **Owner:** Codex
- **Status:** ready
- **Dependency:** none
- **Hardware boundary:** research-only; no controller or PS3 operation

## Objective

Produce a clean-room, source-cited design packet for the future Nintendo USB
initialization state machine and the PS3 adapter boundary. This task writes no
production code.

## Exclusive write scope

- `docs/research/2026-10-22-usb-initialization.md`
- `docs/research/2026-10-22-ps3-adapter-boundary.md`
- this file, for the final handoff

All project source, the UJM checkout and external references are read-only.
Do not edit `README.md`, `DEVLOG.md`, `docs/architecture.md`, protocol code,
tests, build files or the Claude coordination file.

## Work package

1. Pin every researched repository and file to an exact commit.
2. Separate protocol facts, source observations, inferences and open questions.
3. Document the original Pro Controller USB initialization sequence as a
   bounded state machine: commands, expected replies, deadlines, retry limits,
   failure states and detach behavior.
4. Identify endpoint and transfer assumptions that must be discovered at
   runtime rather than hard-coded.
5. Document the minimum PS3 `cellUsbd` transport and `cellPadLdd` injection
   boundary for one `057e:2009` controller and one virtual pad.
6. Record attach/detach, ownership, callback/thread and recovery risks drawn
   from the audited references.
7. Propose a small transport interface for later review, in prose or pseudocode
   only; do not translate or adapt GPL or unlicensed implementation code.

## Acceptance criteria

- Every external claim has a repository, path and pinned commit reference.
- GPL and unlicensed sources remain research-only; no source code is copied,
  translated or mechanically adapted.
- Unknowns are explicit, especially native PS3 Bluetooth and SDK availability.
- The documents clearly distinguish CachyOS validation from PS3 validation.
- No production code, commit, push, installation, console access or hardware
  test is performed.

## Stop conditions

Stop and report instead of guessing if a required source is unavailable, a
protocol claim cannot be corroborated, licensing is ambiguous beyond the
existing audit or the work would require modifying production code.

## Handoff

### 2026-10-22 completion — SP3-CX-01

- **Files written:** `docs/research/2026-10-22-usb-initialization.md` and
  `docs/research/2026-10-22-ps3-adapter-boundary.md`.
- **Sources consulted:** local audit plus pinned commits
  `4403278e4ea2f654f8130a5c03eb8b4516a0f201` (`linux/hid-nintendo.c`),
  `fd98898b856096d441325d51fe5edad7cee19d68` (`multipad`),
  `8925ff15c0b3ba1c1d6f6632178171a4dc728f62` (`PS3xPAD`),
  `05929739baaab0e57b0d8ca3ad5c6407eccb19c9` (`XPAD-Revolution`) and local
  UJM HEAD `75cac25`.
- **Verified:** clean-room separation, source/license boundary, bounded USB
  state-machine proposal, runtime-discovery requirements, one-device/one-pad
  PS3 boundary, and explicit CachyOS/PS3 validation split.
- **Not verified:** real USB timing/replies, SDK availability, PS3 callbacks,
  `cellPadLdd` semantics, native Bluetooth, console behavior, and hardware.
- **Unresolved risks:** exact firmware-dependent USB handshake details, PS3 LDD
  ownership/cancellation semantics, and all target SDK/legal availability.
- **Next exact action:** cross-review these two research documents; then, only
  after approval, implement host-only fixtures/state-machine tests in a separate
  task. Do not implement PS3 or run hardware from this handoff.

### 2026-10-22 correction pass after Claude review

- **Corrections:** added the second post-baud `HANDSHAKE`, separated USB
  `0x81` and subcommand `0x21` response decoding from the input parser,
  clarified source paths/provenance, made `PREPARE` skip-unless-evidence, and
  marked shutdown `EN_TIMEOUT` as an inference/proposal.
- **Verification boundary:** documentation-only; no production code, tests,
  hardware, console, commit or push performed.
- **Remaining gap:** the exact source paths/symbols inside the three PS3
  repositories still require a fresh source-level re-audit before implementation;
  this pass does not claim SDK or PS3 validation.

The completion section above is historical. The research was later integrated
locally by `2aeacc3` before the current reviewer-ownership policy; the exact
source-path and second-handshake claims remain unresolved as documented below.

### 2026-10-22 final review response

- **Changes requested by Claude:** removed the unconfirmed second handshake
  from the executable state-machine proposal; retained it only as an explicit
  open question; removed the false claim that PS3 paths were recorded; removed
  the duplicated sentence; changed both research status lines to
  `locally integrated`; and made the `PREPARE` default unambiguously skip unless
  independently verified evidence exists.
- **Verification:** documentation-only review correction. No source download,
  hardware, console, commit or push performed.
- **Current verdict:** still no green light for M3. The PS3 source-path audit
  and exact second-handshake verification remain required before implementation.

