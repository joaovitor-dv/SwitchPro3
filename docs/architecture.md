# Architecture

## Design constraints

1. The protocol core must compile as freestanding-friendly C11 and must not
   depend on Linux, libusb, the Sony Cell SDK or PSL1GHT.
2. Transport adapters own I/O, timing and thread policy. The core only parses
   or builds bounded byte buffers.
3. The PS3 adapter translates the neutral state into a virtual PS3 pad. It must
   not modify the native Sony controller path.
4. USB is the first hardware milestone. Native Bluetooth is a later transport
   using the same protocol core.
5. Every hardware write must be opt-in, bounded and documented with a recovery
   procedure before it reaches a console.

## Planned layers

```text
Nintendo HID protocol core (portable C)
        |
        +-- Host fixture/USB adapter (CachyOS validation)
        |
        +-- PS3 USB transport + virtual pad adapter (SPRX)
        |
        +-- PS3 native Bluetooth transport (later)
```

## Milestones

| Milestone | Scope | Exit evidence |
|---|---|---|
| M0 | Audit and buildable core skeleton | Host compiler and unit tests pass |
| M1 | Recorded-report parser and calibration | Fixtures cover buttons, sticks and malformed reports |
| M2 | CachyOS USB transport | Original controller handshake and input trace captured |
| M3 | PS3 USB adapter | Recovery-reviewed SPRX build; console test separately authorized |
| M4 | Output features | Player LEDs and rumble validated without input stalls |
| M5 | Native Bluetooth | Internal PS3 Bluetooth path proven without external dongle |

`0x3f` simple input reports, USB handshake output reports, SPI calibration,
rumble and IMU parsing are intentionally outside M0.

