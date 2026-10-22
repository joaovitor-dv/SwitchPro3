# Contributing to SwitchPro3

SwitchPro3 follows the same versioning, documentation and evidence discipline
as the Universal Joystick Manager (UJM), adapted to a PS3 plugin and without
UJM's multi-agent coordination process.

## Ground rules

- Be precise about evidence. State whether a result came from a synthetic
  fixture, a real original Nintendo Switch Pro Controller, a PS3, or reasoning
  only.
- No hardware evidence, no `validated` claim. New paths remain `experimental`
  until a reproducible test is recorded.
- Keep changes small and auditable: one topic per commit.
- Do not include full MAC addresses, serial numbers, console identifiers,
  usernames, IP addresses, credentials or personal paths in reports.
- Do not automate installation, FTP transfer, `boot_plugins.txt` changes,
  firmware writes or console recovery operations without explicit approval.

## Development setup

On CachyOS/Arch Linux:

```fish
sudo pacman -S --needed base-devel git
tools/check.sh
```

`tools/check.sh` is the local source of truth for the checks run by CI. A
skipped check is not a pass.

## How changes get in

- Run `tools/check.sh` before every push.
- Update `README.md`, `docs/` and `DEVLOG.md` in the same commit as the behavior
  they describe.
- Record what was verified and what was not. Host tests do not prove PS3,
  USB-electrical, Bluetooth or in-game behavior.
- Small, finished changes may go directly to `main` after checks. Use a branch
  and Pull Request for large changes, risky PS3 integration or concurrent work.
- Watch CI after every push; fix a red run before continuing.
- Batch real-hardware work through
  [docs/hardware-checklist.md](docs/hardware-checklist.md).

## Commit messages

Use Conventional Commits in English: `feat:`, `fix:`, `docs:`, `test:`,
`chore:`, `build:` and `ci:`. Explain why in the body when it is not obvious,
and mention verification in the commit or Pull Request.

## Versioning

Use SemVer 0.x while USB and Bluetooth support have not completed their
hardware matrices:

| Change | Bump |
|---|---|
| `feat:` new protocol, transport, output feature or PS3 component | minor (`0.x.0`) |
| `fix:` without an interface change | patch (`0.x.y`) |
| Default behavior or compatibility change | patch or minor, by judgment; explain it in the tag message |
| `docs:`, `test:`, `chore:`, `build:`, `ci:` | none |

`1.0.0` is reserved for validated USB and native Bluetooth support on the
documented PS3/CFW matrix, with a recovery-tested installation path.

## Licensing and provenance

SwitchPro3 is MIT-licensed, like UJM. Every source file created by this project
uses `SPDX-License-Identifier: MIT`.

GPL projects such as `multipad` and Linux `hid-nintendo.c`, and source trees
without an explicit license such as the audited PS3XPAD derivatives, are
research references only. Protocol facts may be independently implemented;
their code must not be copied, translated or mechanically adapted into this
repository. Any exception requires an explicit license review and project-wide
decision before code is written.

## Cutting a release

1. Confirm `tools/check.sh` and CI are green on the exact commit.
2. Record the applicable hardware-checklist results. Never convert untested
   behavior into a validated claim.
3. Obtain explicit maintainer approval before publishing.
4. Add the release entry to `DEVLOG.md`, update the README title and Patch
   notes, and update future package/plugin version fields in the same commit.
5. Create an annotated tag:
   `git tag -a vX.Y.Z -m "vX.Y.Z - <title>"`.
6. Push the branch and tag, then create a GitHub pre-release while version is
   0.x. Attach source/binaries and `SHA256SUMS` when build artifacts exist.

