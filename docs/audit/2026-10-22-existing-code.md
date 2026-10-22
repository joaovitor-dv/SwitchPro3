# Existing-code audit — 2026-10-22

## Scope

This audit covers the initial references for SwitchPro3. Repositories were
inspected in temporary read-only clones. No source was copied into SwitchPro3.
The UJM checkout had unrelated uncommitted work and was treated as strictly
read-only; its recorded hash identifies HEAD, not a clean working tree.

| Reference | Audited commit | License observed | Useful evidence | Reuse decision |
|---|---|---|---|---|
| `esc0rtd3w/multipad` | `fd98898b856096d441325d51fe5edad7cee19d68` | GPL-3.0 | PS3 kernel/user split, `cellUsbd` path, virtual-pad bridge, preliminary Switch parsing | May be adapted only under GPL with attribution and provenance |
| `aldostools/PS3xPAD` | `8925ff15c0b3ba1c1d6f6632178171a4dc728f62` | No repository license found | Historical VSH SPRX, USB LDD and `cellPadLdd` architecture | Reference only; do not copy |
| `GabaRSk/XPAD-Revolution` | `05929739baaab0e57b0d8ca3ad5c6407eccb19c9` | No license for the PS3 core; notices cover viewer dependencies only | Maintained PS3xPAD derivative, ring buffers, detach safety, game/VSH separation | Reference only; do not copy |
| Linux `hid-nintendo.c` | `4403278e4ea2f654f8130a5c03eb8b4516a0f201` | GPL-2.0-or-later | Canonical report IDs, USB commands, calibration, parsing, LEDs and rumble | Protocol facts may guide a clean implementation; copied/adapted code must retain GPL provenance |
| Universal Joystick Manager | local `75cac25` | MIT | Original-controller identification and CachyOS validation workflow | Read-only external reference; no modification or vendoring |

## Findings

### 1. The portable core is the correct first boundary

Nintendo report parsing is independent of PS3 APIs. Keeping it in a small C11
library lets recorded fixtures run under sanitizers and ordinary host compilers
before any code is loaded into VSH.

### 2. USB support needs an explicit initialization state machine

The original Pro Controller (`057e:2009`) does not reduce to a passive generic
HID reader. The audited Linux driver defines Nintendo USB command reports,
including handshake, baud-rate selection and timeout control, followed by
subcommands such as report-mode selection. SwitchPro3 must model these as
bounded states with retries and deadlines rather than placing blocking I/O in
the input pump.

### 3. PS3 virtual-pad injection is the highest-risk layer

The PS3 references use `cellUsbd` LDD callbacks for transport and
`cellPadLddDataInsert` for virtual input. Their histories document LDD table
limits, duplicate registrations, detach races, stalled input threads and VSH
freezes. The first PS3 build must therefore support one VID/PID only, one pad,
no game-process injection and no Bluetooth.

### 4. Bluetooth must remain a later milestone

PS3XPAD's wireless path targets an external USB Bluetooth adapter. `multipad`
contains an experimental LV2 Bluetooth HID observation path, but that is not
evidence of a production-safe native Bluetooth transport. Internal-module
support requires separate reverse engineering and hardware evidence.

### 5. Licensing must stay visible in architecture decisions

PS3XPAD and XPAD-Revolution have no explicit license covering their PS3 source
in the audited trees. Public source availability is not permission to copy.
SwitchPro3 therefore uses original code and protocol facts, with GPL-compatible
licensing selected for future, attributable integration of GPL references.

## Immediate risks

| Risk | Consequence | M0/M1 control |
|---|---|---|
| Malformed or short HID report | Out-of-bounds access inside VSH | Size checks before every field read; host negative tests |
| Incorrect axis calibration | Drift, reduced range or stuck movement | Preserve raw 12-bit values until calibration is implemented |
| Blocking output command | Input loss or frozen VSH | Separate future transport state machine from parser |
| LDD ownership conflict | Controller disappears or USB table fills | One exact VID/PID and symmetric attach/detach in first PS3 adapter |
| Unlicensed code reuse | Redistribution cannot be justified | No copying from PS3XPAD derivatives; provenance table for every imported unit |

