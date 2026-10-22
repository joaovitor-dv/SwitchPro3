# SwitchPro3

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![Status: pre-release](https://img.shields.io/badge/status-pre--release-orange)

SwitchPro3 is an independent controller plugin project for using an original
Nintendo Switch Pro Controller on a PlayStation 3. The first target is a PS3
Slim CECH-2001A running Evilnat CFW.

The project is deliberately split into two layers:

- a portable C11 library for Nintendo HID protocol handling;
- thin platform adapters for host-side validation and the eventual PS3 SPRX.

The Universal Joystick Manager (UJM) remains an external reference and is not
modified or embedded by this project.

SwitchPro3 follows UJM's project discipline: SemVer 0.x, English Conventional
Commits, documentation and devlog updates in the same commit as behavior, one
local check entry point, explicit hardware evidence and annotated release tags.

## Current status

- Existing projects and licenses have been audited and pinned by commit.
- Report `0x30` common input data is parsed into a transport-neutral state.
- Host tests build with the system C compiler on CachyOS.
- USB transport, PS3 virtual-pad injection, rumble and Bluetooth are not yet
  implemented.

## Build and test on CachyOS

The base development tools are sufficient:

```fish
sudo pacman -S --needed base-devel git
make test
```

The project does not install anything on the host or the PS3.

## Safety boundary

No PS3 installation, FTP transfer, boot plugin edit, firmware modification or
hardware test is automated by this repository. Those operations require a
separate, explicit procedure and recovery plan.

See [the architecture](docs/architecture.md) and
[the initial source audit](docs/audit/2026-10-22-existing-code.md). Development
and release rules are in [CONTRIBUTING.md](CONTRIBUTING.md).

## Patch notes

| Version | Summary |
|---|---|
| Unreleased | Portable HID core and host-test foundation; no PS3 hardware support yet |

