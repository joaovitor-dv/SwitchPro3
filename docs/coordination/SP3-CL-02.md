# Claude task — M1 host evidence: calibration, reply decoders, fixtures

## Assignment

- **Task ID:** `SP3-CL-02` (proposed by Claude; needs Codex review before it starts)
- **Owner:** Claude
- **Status:** proposed, revised after Codex review — Part A ready after re-review, Part B blocked on `SP3-CX-02`
- **Dependency:** Part B needs `SP3-CX-02` (below)
- **Hardware boundary:** host-only; no controller, no USB I/O, no PS3

## Objective

Reach milestone M1 (recorded-report parser, calibration, malformed-input coverage) with synthetic
fixtures only. Add the bounded reply decoders that the USB research requires (`0x81` USB response,
`0x21` subcommand reply) separately from the input parser. No transport, no state machine, no output
reports, no PS3 symbols.

## Prerequisite task for Codex: `SP3-CX-02` (protocol facts)

The decoders and the calibration source need packet-layout facts that are not in the repository:

1. layout of the USB response report `0x81` (command echo, status, length);
2. layout of the `0x21` reply beyond the 12-byte common prefix (ACK/NACK byte, subcommand ID,
   payload offset) and the `REQ_DEV_INFO` payload size;
3. SPI calibration source: flash addresses and layout of stick calibration (user vs. factory),
   including the 12-bit packing and the sentinel for "no user calibration".
4. the second `HANDSHAKE` question from `docs/research/2026-10-22-usb-initialization.md`
   (informational for M2; not needed here).

Each fact needs repository, path, function/line and the pinned commit
(`4403278e4ea2f654f8130a5c03eb8b4516a0f201`), written as a clean-room addendum under `docs/research/`.
Reading the pinned source requires fetching it: the download was authorized for Codex's clean-room research only (no copy, no adaptation, nothing added to
the public repository); the source stays research-only (GPL-2.0-or-later). Until `SP3-CX-02` lands, Part B does not start.

## Part A — ready after review

Exclusive write scope: `include/switchpro3/calibration.h`, `src/calibration.c`,
`tests/test_calibration.c`, `tests/fixture_loader.h` (test-only), `tests/fixtures/`, `Makefile` (new
objects/targets only), `tools/mutation-check.sh` or a sibling `tools/mutation-check-calibration.sh`, `DEVLOG.md`.

### A1. Calibration contract (normative)

```c
struct sp3_axis_calibration {
    uint16_t min, center, max;   /* raw 12-bit samples */
    uint16_t deadzone;           /* raw sample units, see below */
};
enum sp3_cal_status { SP3_CAL_OK = 0, SP3_CAL_ERROR_ARGUMENT = -1, SP3_CAL_ERROR_RANGE = -2,
                      SP3_CAL_ERROR_DEADZONE = -3, SP3_CAL_ERROR_SAMPLE = -4 };
enum sp3_cal_status sp3_axis_calibrate(const struct sp3_axis_calibration *cal,
                                       uint16_t raw, int16_t *out);
```

- **Output type and range:** `int16_t` in the symmetric closed range `[-32767, 32767]`. `-32768` is never
  produced, so negation is always safe. Positive full scale is `+32767`, negative full scale `-32767`.
- **Validation of the calibration** (checked before any use, `SP3_CAL_ERROR_RANGE`): all of `min`, `center`, `max`
  are `<= 0x0fff` and `min < center < max`.
- **Dead zone:** unit is **raw samples**, applied **before** normalization, around `center`. Let
  `d = raw - center`. If `|d| <= deadzone` the output is exactly `0` (edge inclusive). Otherwise the
  remaining travel is rescaled so the output is continuous at the edge:
  `positive: out = (d - deadzone) * 32767 / (max - center - deadzone)`,
  `negative: out = -((-d - deadzone) * 32767 / (center - min - deadzone))`, integer division truncating
  toward zero, then saturated to `+32767`/`-32767`. `raw >= max` gives `+32767`, `raw <= min` gives `-32767`.
  Intermediate products use `int32_t` (max `4095 * 32767`, no overflow).
- **Dead zone validity** (`SP3_CAL_ERROR_DEADZONE`): `deadzone < center - min` **and**
  `deadzone < max - center`, so each side keeps at least one raw step of travel. `deadzone == 0` is valid.
  A `uint16_t` cannot be negative; the check is still on the unsigned value.
- **Raw sample** above `0x0fff` returns `SP3_CAL_ERROR_SAMPLE`. A NULL `cal` or `out` returns
  `SP3_CAL_ERROR_ARGUMENT`. **Every error leaves `*out` unchanged.**
- **Properties tested over all 4096 samples** for several calibrations (symmetric, asymmetric, `deadzone`
  zero and non-zero): output within `[-32767, 32767]`, monotonic non-decreasing in `raw`, `center` maps to
  `0`, `min` to `-32767`, `max` to `+32767`.

### A2. Required table cases

Calibrations: asymmetric (`min=500, center=2000, max=3800`, margin on both ends) and symmetric
(`min=1, center=2048, max=4095`, travel 2047 on both sides), each with `deadzone` zero and non-zero (for
example `0` and `100`).

- **In-domain samples (saturation, not errors):** `center`, `min`, `max`, `min-1` and `max+1` **whenever the
  value is still `<= 0x0fff` and `>= 0`** (asymmetric: `499` and `3801` saturate to `-32767`/`+32767`;
  symmetric: `min-1 = 0` saturates, but `max+1` is not a valid sample, see next point).
- **Out-of-domain samples (error, not saturation):** any sample `> 0x0fff` returns `SP3_CAL_ERROR_SAMPLE`.
  With `max == 0x0fff` the sample `0x1000` is the `max+1` case and tests the domain error only.
- **Dead zone edge, each side, for `deadzone > 0`:** `deadzone-1` and `deadzone` distance from `center` give
  `0`; `deadzone+1` gives the first non-zero value. **For `deadzone == 0`:** `deadzone-1` is omitted (a distance
  of `-1` is not defined); the cases are `0` (gives `0`) and `+1`/`-1` from `center` (give the first non-zero
  value on each side). Edge inclusivity is therefore only exercised by the non-zero dead zone.
- **Invalid calibrations:** `min==center`, `center==max`, `min>center`, any field `>0x0fff`, `deadzone` equal to
  and above each side's travel (checked on the smaller side as well as the larger one). Each asserts
  `SP3_CAL_ERROR_*` and that a non-NULL `*out` is unchanged.
- **NULL pointers:** `cal == NULL` and `out == NULL` return `SP3_CAL_ERROR_ARGUMENT`. For `out == NULL` the test
  checks only the status code and that nothing is dereferenced (the run is under ASan/UBSan); preservation of
  `*out` applies only to a non-NULL `out`, including the `cal == NULL` case.

### A3. Fixture contract (test-only)

- **Format:** text, one report per non-empty line. Tokens are exactly two hex digits (`[0-9a-fA-F]{2}`),
  separated by one or more spaces or tabs; no `0x` prefix. `#` starts a comment to end of line (full-line or
  trailing). Blank and comment-only lines are ignored.
- **Limits:** line length at most 1024 characters, report length at most 64 bytes, at least one report per
  file. Bytes are therefore always `0x00..0xff`.
- **Errors:** an odd digit count, a non-hex character, a token of the wrong length, an over-long line or report,
  or an empty file makes the loader return an error carrying file (or buffer name) and line number; the test
  fails with that message. The loader never accepts partial data.
- **Placement:** the loader is `static` functions in `tests/fixture_loader.h`, included only by tests, and takes a
  string buffer so it is also tested without files (accepting and rejecting cases). Files in `tests/fixtures/`
  are named `synthetic-*.hex` and start with a comment saying they are synthetic, not captured.

### A4. Mutation harness

The calibration code is new code: ship reproducible mutants (a script that applies each to a scratch copy and
requires the tests to fail, 0 survivors, refusing a mutation that does not apply or compile). Required mutants
include: wrong edge comparison (`<=` to `<`), wrong rescale divisor, negative side using the positive span,
missing saturation, deadzone applied after normalization, validation `<` changed to `<=`, and `*out` written
before validation.

## Part B — blocked on `SP3-CX-02`

Exclusive write scope: `include/switchpro3/reply.h`, `src/reply.c`, `tests/test_reply.c`, `Makefile`,
`DEVLOG.md`.

1. `sp3_decode_usb_response()` — accepts only report `0x81`, validates the minimum length, returns the
   command and status; rejects truncation without changing the caller's state.
2. `sp3_decode_subcmd_reply()` — accepts only report `0x21`, validates ACK/NACK, subcommand ID and
   payload length for the subcommands used (`0x02`, `0x03`); same rejection rules.
3. Tests for every truncated length, wrong report ID, mismatched command/subcommand, trailing bytes, and
   a mutation harness.

## Acceptance criteria

- Freestanding-friendly C11; no platform header, no dynamic allocation, no I/O in the library.
- `tools/check.sh` passes; mutation check shows 0 survivors; ASan/UBSan run reported separately.
- CI green on at least two compilers locally if available, otherwise state which one was used.
- `DEVLOG.md` separates synthetic fixtures from real-controller evidence; no hardware claim.
- No file copied or mechanically adapted from GPL or unlicensed sources; every layout fact traces to the
  `SP3-CX-02` addendum.
- Per policy, the author leaves the work uncommitted; the reviewer integrates, commits and pushes after a
  complete green light.

## Stop conditions

Stop and report if a layout fact is missing or contradictory, if the work needs a real controller, USB
I/O, PS3 symbols or files outside the write scope, or if reuse of GPL/unlicensed code seems necessary.

## Handoff

Append a completion section here: commits (none, per policy), files, commands, results, unresolved risks
and the next exact action.

## Part B contract draft (for review; offsets come only from the `SP3-CX-02` addendum)

The shapes below do not depend on packet offsets. Every numeric layout constant (minimum sizes, field
offsets, payload lengths) is defined from the addendum with its citation; none is invented here.

```c
enum sp3_reply_status { SP3_REPLY_OK = 0, SP3_REPLY_ERROR_ARGUMENT = -1, SP3_REPLY_ERROR_SHORT = -2,
                        SP3_REPLY_ERROR_UNSUPPORTED_REPORT = -3, SP3_REPLY_ERROR_MISMATCH = -4,
                        SP3_REPLY_ERROR_PAYLOAD = -5 };

struct sp3_usb_response { uint8_t command; uint8_t status; };      /* meaning of status: addendum */
struct sp3_subcmd_reply { uint8_t ack; uint8_t subcommand;
                          const uint8_t *payload; size_t payload_length; };

enum sp3_reply_status sp3_decode_usb_response(const uint8_t *report, size_t size,
                                              uint8_t expected_command, struct sp3_usb_response *out);
enum sp3_reply_status sp3_decode_subcmd_reply(const uint8_t *report, size_t size,
                                              uint8_t expected_subcommand, struct sp3_subcmd_reply *out);
```

- **Check order** (same as the input parser): NULL arguments, report ID (`0x81` / `0x21`), minimum length,
  command/subcommand mismatch, then payload rules. Every error leaves `*out` unchanged; for `out == NULL`
  only the status is checked.
- `payload` points into the caller's buffer and is valid only as long as that buffer; the decoder never copies
  or retains it, and never reads past `size`.
- **ACK/NACK:** a NACK is reported as `SP3_REPLY_OK` with `ack` carrying the raw byte, so the state machine
  decides policy; the decoder rejects only malformed frames. (If the addendum shows a NACK cannot be
  distinguished without payload context, this line is revisited in review.)
- **Tests:** every length from 0 to the largest accepted size + 1, every other report ID, mismatched command
  and subcommand, trailing bytes, `REQ_DEV_INFO` payload length rules (from the addendum), `out == NULL`;
  synthetic fixtures only, named `synthetic-*.hex`.
- **Mutation harness:** a sibling script, same rules as the others (0 survivors, refuse non-applying or
  non-compiling mutants), covering at least: wrong report ID, off-by-one length limit, mismatch check removed,
  write before validation, payload length unchecked, payload pointer off by one.
- Write scope as in Part B above plus `tests/fixtures/` and the new harness script.
