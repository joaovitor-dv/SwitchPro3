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

Append a short completion section here containing files written, sources and
commits consulted, verification performed, unresolved risks and the next exact
action. Leave the research changes uncommitted for review and integration.

