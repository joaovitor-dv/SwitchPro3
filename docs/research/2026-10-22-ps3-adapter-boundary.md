# PS3 adapter boundary — clean-room research

Status: research-only, locally integrated, no PS3 or controller access.

## Evidence boundary

The local audit pins the following read-only references. Exact source paths and
symbols for the PS3 repositories were not confirmed in this pass; the entries
below must not be read as source-level provenance or implementation inputs.

| Repository | Commit | Evidence used | License boundary |
|---|---|---|---|
| `esc0rtd3w/multipad` | `fd98898b856096d441325d51fe5edad7cee19d68` | repository-level audit only; exact path/symbol not confirmed | GPL-3.0; research only |
| `aldostools/PS3xPAD` | `8925ff15c0b3ba1c1d6f6632178171a4dc728f62` | repository-level audit only; exact path/symbol not confirmed | no repository license observed; research only |
| `GabaRSk/XPAD-Revolution` | `05929739baaab0e57b0d8ca3ad5c6407eccb19c9` | repository-level audit only; exact path/symbol not confirmed | no license covering PS3 core observed; research only |
| Universal Joystick Manager | local HEAD `75cac25` | device identity and CachyOS workflow | MIT; read-only |

No code from these repositories is included or mechanically adapted.

## Minimum one-device / one-pad boundary

The smallest defensible future PS3 adapter has two independently testable
halves:

1. A `cellUsbd`-side transport that claims exactly one matching `057e:2009`
   device, discovers the interface and endpoints, owns one transfer context,
   forwards input reports to the neutral parser, and serializes output commands.
2. A `cellPadLdd`-side publisher that registers exactly one virtual pad and
   injects snapshots only after the parser has produced a complete neutral
   state. It must not inject into a game process, load a second pad, or attempt
   Bluetooth.

The adapter boundary is therefore a queue plus lifecycle contract, not a direct
call from a USB callback into a pad API:

```text
cellUsbd attach -> transport context -> bounded report queue
                                     -> neutral parser
                                     -> one virtual-pad snapshot
                                     -> cellPadLdd publisher
```

## Ownership and lifecycle contract

* Attach is accepted only for the exact VID/PID after descriptor validation.
* Registration failure, including a full LDD table, is reported without
  retaining partial ownership.
* USB callbacks do minimal work: capture status and bytes, enqueue, and return.
  Parsing, pad publication and recovery run outside the callback context.
* Detach first stops new transfers and publication, then cancels transfers,
  marks the context dead, drains or discards queued reports, unregisters the
  virtual pad, and finally releases memory. Late callbacks must be harmless.
* A new attach receives a new generation/context token; callbacks from an old
  generation cannot publish input.
* A stalled endpoint, malformed report, timeout, or callback error causes a
  bounded recovery transition, not an unbounded retry or VSH-wide reset.

These are safety requirements inferred from the local audit and its recorded
detach/LDD risks; they are not proof that the public references are safe to
deploy on a PS3. The local audit and pinned commits identify repositories, but
do not yet provide source-level path/symbol provenance. That re-audit is a
hard prerequisite for any M3 implementation task.

## Proposed transport interface (prose only)

The later MIT implementation should expose only operations equivalent to:

* `open(descriptor_snapshot)` — validate identity and create a generation;
* `submit_output(command)` — serialize one bounded output transaction;
* `poll_input()` — return immutable report bytes/status to the portable core;
* `recover(reason)` — cancel and reinitialize the current generation;
* `close()` — make callbacks inert and release ownership.

The PS3 publisher should consume neutral snapshots through `publish(snapshot)`
and `withdraw(reason)`. Neither side should know the other's SDK details.

## PS3-specific unknowns and risks

* Availability, version and legal build access for the Sony Cell SDK or PSL1GHT
  are not established by this research.
* Exact `cellUsbd` callback signatures, LDD registration limits, transfer
  cancellation semantics and unload ordering require a target-compatible SDK
  and review.
* Exact `cellPadLdd` registration/injection contract, button numbering,
  pressure fields and PS-button carrier behavior are not established here.
* Native PS3 Bluetooth is explicitly out of scope. An external USB Bluetooth
  adapter reference is not evidence for the internal radio path.
* VSH freezes, duplicate registration, stale callbacks and game/VSH ownership
  conflicts are high-impact failure modes. No console test was performed.

## Validation split

| Boundary | Validated by this packet | Still requires separate authorization |
|---|---|---|
| CachyOS USB | source-based sequence, proposed state machine and host-test targets | real controller traces and timing |
| Portable parser | interface shape and malformed-input requirements | implementation and fixtures |
| PS3 `cellUsbd` | conceptual ownership boundary only | SDK build, target load and USB attach test |
| PS3 `cellPadLdd` | conceptual one-pad publisher only | target injection and game/VSH safety test |
| Native Bluetooth | none | independent reverse engineering and hardware evidence |

