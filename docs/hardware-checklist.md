# Hardware checklist

This checklist contains tests that host fixtures and CI cannot prove. Run only
the steps that match the current milestone, in order, and record exact results.
Hardware execution, PS3 installation and configuration changes require
separate explicit approval.

## Required equipment

- one original Nintendo Switch Pro Controller (`057e:2009`);
- a known data-capable USB cable;
- the CachyOS development host;
- for PS3 milestones, the target PS3 Slim CECH-2001A with the documented
  Evilnat CFW version and a tested recovery path.

## 0. Pin the code under test

Record the commit and prove the local checks pass:

```fish
git rev-parse HEAD
tools/check.sh
```

Keep the commit hash and full output. Do not describe a different working tree
as the tested version.

## 1. Passive CachyOS identification

Before sending Nintendo output reports, connect the controller by USB and
record only non-sensitive descriptors: VID/PID, interface, endpoint directions,
packet sizes and active kernel driver. Mask serial numbers and physical paths.

Expected: original Nintendo `057e:2009`; no detach, reset or output command.

## 2. CachyOS USB initialization

After the transport state machine and its fixture tests exist, run one bounded
handshake with per-step deadlines. Record report IDs, lengths, return codes and
timing without serial numbers or addresses.

Stop immediately on repeated timeouts, USB resets, kernel errors, controller
disconnect loops or loss of other USB devices.

## 3. Host input matrix

Verify every button, D-pad direction and both stick axes. Repeat connect,
disconnect and reconnect at least three times. Record neutral/minimum/maximum
stick values; do not label calibration validated from synthetic fixtures.

## 4. Passive PS3 detection

With a recovery-reviewed debug build, verify only plugin load, exact VID/PID
matching and clean attach/detach. Do not inject virtual pad data yet. Keep the
plugin log and confirm native Sony controllers remain unaffected.

## 5. PS3 USB input

Enable one controller and one virtual pad only. Verify XMB navigation and a
small game sample, including every button, D-pad and stick. Record CFW version,
plugin commit, number of runs and any input stalls.

## 6. Soak and recovery

Use the controller for 30 minutes, then perform at least ten cable reconnects
and one plugin unload/reload cycle. Confirm no VSH freeze, no growing USB/LDD
allocation and no effect on a native DualShock 3.

Prove the documented recovery path removes the plugin from boot without data
loss. A build cannot be called release-ready until this step is recorded.

## 7. Output features

LEDs and rumble are separate gates. Test them only after input is stable, and
prove output traffic does not stall the input pump.

## 8. Native Bluetooth

Bluetooth is a later milestone. Do not infer internal-radio support from a USB
Bluetooth adapter, Linux behavior or passive LV2 report visibility. Record
pairing, reconnect, input, coexistence with native pads, soak and recovery as
separate results.

## Result table

| Step | Commit | Hardware/firmware | Runs/time | Result | Evidence path |
|---|---|---|---|---|---|
| 1 Passive host identification | | | | | |
| 2 Host initialization | | | | | |
| 3 Host input matrix | | | | | |
| 4 Passive PS3 detection | | | | | |
| 5 PS3 USB input | | | | | |
| 6 Soak and recovery | | | | | |
| 7 Output features | | | | | |
| 8 Native Bluetooth | | | | | |

