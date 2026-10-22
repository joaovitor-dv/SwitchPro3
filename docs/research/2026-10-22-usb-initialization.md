# Nintendo Pro Controller USB initialization — clean-room research

Status: research-only, uncommitted, no hardware exercised.

## Evidence boundary

The protocol observations below are derived from Linux
`drivers/hid/hid-nintendo.c` at commit
`4403278e4ea2f654f8130a5c03eb8b4516a0f201` (GPL-2.0-or-later), especially
`joycon_send_usb()`, `joycon_send_subcmd()`, `joycon_hid_send_sync()`, the USB
command definitions, and the controller initialization path. The file is an
external reference only; no source was copied, translated, or adapted.
The file is an external reference only; no source was copied, translated, or
adapted. The existing audit also records UJM HEAD `75cac25` as MIT and read-only.

## Facts and source observations

* The original device is identified as Nintendo `057e:2009`.
* USB output command reports are distinct from subcommand reports. The audited
  source names USB output `0x80`, USB response `0x81`, and the USB commands
  `CONN_STATUS (0x01)`, `HANDSHAKE (0x02)`, `BAUDRATE_3M (0x03)`,
  `NO_TIMEOUT (0x04)`, `EN_TIMEOUT (0x05)`, `RESET (0x06)`,
  `PRE_HANDSHAKE (0x91)` and `SEND_UART (0x92)`.
* Subcommand output is `0x01`; the input subcommand reply is `0x21`.
  The relevant initialization subcommands are `SET_REPORT_MODE (0x03)` and
  `REQ_DEV_INFO (0x02)`; normal full input reports use `0x30`.
* The driver waits for a matching input response after a command and has a
  bounded synchronous timeout/retry path. It also waits up to a quarter second
  for an input report before a subcommand in read mode, but proceeds after that
  wait expires. These are observations of the GPL reference, not requirements
  for SwitchPro3.
* In the audited initialization path, changing to the 3 Mbaud mode is followed
  by a second `HANDSHAKE` before timeout-mode setup. The second handshake is
  therefore a distinct state in the proposed machine; it is not a retry of the
  first handshake.

## Bounded state machine proposal

This is design prose for later review, not an implementation claim.

| State | Action | Expected evidence | Proposed bound | Failure transition |
|---|---|---|---|---|
| `DETACHED` | No I/O; clear transport ownership | Device absent | immediate | `ATTACHED` on runtime attach |
| `ATTACHED` | Read descriptors and discover HID interface/endpoints | Valid HID interface, interrupt IN and usable OUT path | one descriptor transaction window | `FAILED_DESCRIPTOR` |
| `PREPARE` | Send USB pre-handshake if the discovered path requires it | Matching `0x81` response or documented device reply | 250 ms, 2 attempts | `RETRY` then `FAILED_INIT` |
| `HANDSHAKE_1` | Send USB handshake | Matching USB response for command `0x02` | 500 ms, 3 attempts | `RETRY` then `FAILED_INIT` |
| `BAUDRATE` | Request 3 Mbaud mode | Matching response for `0x03` | 500 ms, 3 attempts | `RETRY` then `FAILED_INIT` |
| `HANDSHAKE_2` | Re-establish the command link after the baud-rate change | Matching USB response for command `0x02` | 500 ms, 3 attempts | `RETRY` then `FAILED_INIT` |
| `TIMEOUT_MODE` | Disable device timeout for setup; any later `EN_TIMEOUT` is a proposal only | Matching `0x04` response | 500 ms, 3 attempts | `RETRY` then `FAILED_INIT` |
| `REPORT_MODE` | Send subcommand `0x03` selecting report `0x30` | `0x21` reply with matching counter/subcommand | 1 s, 3 attempts | `RETRY` then `FAILED_INIT` |
| `DEVICE_INFO` | Request subcommand `0x02` | Valid `0x21` reply and device-info payload | 1 s, 3 attempts | `RETRY` then `FAILED_INIT` |
| `READY` | Consume input reports; serialize output commands | Valid report IDs and lengths | continuous | `DETACHING` or `FAILED_RUNTIME` |
| `FAILED_INIT` | Stop output, release interface, record reason | No further ownership | no retry loop | `DETACHED` |
| `DETACHING` | Cancel pending transfers, ignore late callbacks, release ownership | No callback accesses freed state | bounded cancellation | `DETACHED` |

The timeouts and retry counts in the table are conservative implementation
proposals. They must be measured against a real controller on CachyOS before
being treated as protocol facts. A failed command must not be retried forever;
the transport must expose the reason and detach cleanly.

`PREPARE` is now testable: the default path is to skip it and proceed to
`HANDSHAKE_1`; enter `PREPARE` only when descriptor/runtime evidence or a
future captured trace explicitly requires `PRE_HANDSHAKE (0x91)`. The adapter
must record which branch was selected. `EN_TIMEOUT (0x05)` on shutdown is an
inference/proposal, not an observation from the cited source, and must remain
disabled until a host trace proves it is required and safe.

USB acknowledgements and subcommand replies are not part of the portable input
parser. A separate bounded response decoder is required:

* `decode_usb_response()` accepts only report `0x81`, validates the minimum
  length and matches the response to the outstanding USB command.
* `decode_subcmd_reply()` accepts only report `0x21`, validates the ACK/NACK
  byte, subcommand ID, packet counter association and the command-specific
  payload length. `REQ_DEV_INFO (0x02)` must have its own payload-length and
  controller-type checks.
* Both decoders must reject truncation without changing caller state and must
  be tested independently from `0x30`/`0x31` input parsing. The existing
  parser-hardening commit `cbacfc0` does not provide this coverage.

## Runtime discovery requirements

The future adapter must discover, rather than hard-code, the HID interface
number, alternate setting, interrupt IN/OUT endpoint addresses, max packet
sizes, polling intervals, report descriptor length, and whether the OUT path is
an interrupt transfer or a HID-class output transfer. `057e:2009` is an
identity filter, not proof that every descriptor layout or kernel path is
identical. Report IDs, report lengths, response counters and subcommand IDs
must be checked before field access.

## Detach and recovery

Detach is a terminal event for the current transport instance: cancel pending
transfers, prevent new output, invalidate callbacks, release the interface,
and discard queued reports. A timeout, malformed reply, USB reset, or endpoint
stall must enter a bounded failure path and permit a fresh attach instance.
No blocking command belongs in the input parser or PS3 callback thread.

## CachyOS versus PS3 validation

CachyOS can validate enumeration, descriptors, report traces, timing, command
replies, malformed reports, and clean detach using host fixtures and an
authorized controller test. None of that validates `cellUsbd`, VSH ownership,
`cellPadLdd`, or PS3 scheduling. PS3 validation remains a separately
authorized hardware milestone.

## Open questions

1. Does any original-controller firmware require `PRE_HANDSHAKE (0x91)`? The
   default branch is now skip-unless-evidence.
2. What exact reply payload and counter behavior should be accepted after each
   USB command, including reset and endpoint stall recovery?
3. What timing margins are safe for the intended CachyOS USB stack?
4. Can the PS3 USB stack issue the required HID output transfers without
   changing ownership of the device?

