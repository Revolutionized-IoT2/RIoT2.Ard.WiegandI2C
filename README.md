# RIoT2.Ard.WiegandI2C

Standalone ATtiny85 Arduino sketch that decodes a 26-bit Wiegand card reader and exposes the
decoded 24-bit code over I2C. The M5Core2 and M5Dial firmware (0.6.0 and newer) read it with the
shared `WiegandI2CPeripheral` ([I42](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-3-4.md#i42-wiegand-peripheral-s)): plug the module into the board's Grove PORT.A and
add the *Wiegand Reader* peripheral to the node's configuration; every card read becomes a text
report. Parameters and formats:
[RIoT2.Ard.Shared](https://github.com/Revolutionized-IoT2/RIoT2.Ard.Shared#firmware-behavior-supplied-here).

## Hardware

| Signal | ATtiny85 pin | Notes |
|---|---|---|
| Wiegand D0 | `PB3` | Input, pin-change interrupt |
| Wiegand D1 | `PB4` | Input, pin-change interrupt |
| Data-ready IRQ | `PB1` | Output, high when an unread code is available |
| I2C SDA | `PB0` | USI I2C slave, external pull-up required |
| I2C SCL | `PB2` | USI I2C slave, external pull-up required |

Use a common ground between the reader, bridge and host. Power the reader from its rated supply;
do not feed a 12 V reader supply into the ATtiny.

## I2C protocol

- Slave address: `0x26`.
- Read length: 3 bytes, most-significant byte first.
- Payload: low 24 bits of a valid Wiegand26 frame, with parity bits stripped.
- Empty result: `00 00 00` when no new code is pending.
- A successful request queues and consumes the pending code; the IRQ line is dropped.
- Short or aborted reads do not preserve the consumed code for retry.

Only Wiegand26 is accepted. Frames with the wrong length or invalid Wiegand26 parity are discarded.
If a complete unread code is already pending, later complete frames are dropped and counted by the
local `wiegandDroppedFrames()` diagnostic.

## Build and flash

Build with PlatformIO from the workspace root: `pio run -d RIoT2.Ard.WiegandI2C` writes
`.pio/build/attiny85/firmware.hex`, and `-t program` flashes it with a USBasp ISP programmer. CI
builds it on every push, and a `x.y.z` tag attaches `firmware-attiny85-<tag>.hex` to a GitHub
release.

Or use the Arduino IDE:

1. Install an ATtiny board package such as ATTinyCore in the Arduino IDE.
2. Open [WiegandI2C.ino](WiegandI2C.ino).
3. Select an ATtiny85 target using the 8 MHz internal clock.
4. Burn bootloader/fuses once for a fresh chip.
5. Flash with an ISP programmer using **Upload Using Programmer**.

The sketch uses the local `TinyWireS` and `usiTwiSlave` files in this repository; no RIoT2 shared
library is required.

## Host regression tests

With sibling firmware repositories checked out, Python and a native C++14 compiler available, run
from [RIoT2.Ard.Shared](https://github.com/Revolutionized-IoT2/RIoT2.Ard.Shared):

```powershell
Set-Location C:\Src\RIoT2\RIoT2.Ard.Shared
python .\tests\test_firmware_p1.py
python .\tests\test_firmware_p2.py
```

The tests compile the production USI driver and sketch callbacks with fake AVR registers. They
exercise short reads, repeated START recovery, TX overflow, normal reads, invalid frames and
pending-code preservation during acquisition. They do not replace a real ATtiny board build and
hardware timing test.

## Wiring summary

```text
Wiegand reader D0  -> ATtiny85 PB3
Wiegand reader D1  -> ATtiny85 PB4
Wiegand reader GND -> common ground
Wiegand reader V+  -> reader rated supply

ATtiny85 PB0 (SDA) -> host SDA with pull-up to logic rail
ATtiny85 PB2 (SCL) -> host SCL with pull-up to logic rail
ATtiny85 PB1 (IRQ) -> host GPIO, optional active-high data-ready signal
```

On the M5 boards, SDA and SCL are Grove PORT.A (A1 and A2) and the IRQ goes to B1 or B2. The ESP32
pins aren't 5 V tolerant: run the ATtiny from 3.3 V (fine at its 8 MHz internal clock), or
level-shift the bus and IRQ.

## Limits

- Wiegand26 only; Wiegand34/37 and other lengths are rejected.
- No I2C `onReceive` handler exists, so the slave is read-only.
- Only one completed code is queued. Additional completed frames are counted and dropped until the
  host reads the pending code.

## Contributing

- Instructions for AI coding agents: [AGENTS.md](AGENTS.md).
- Release notes: [CHANGELOG.md](CHANGELOG.md).
- Platform documentation hub: [.github/docs](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/README.md).

## License

See [LICENSE](LICENSE).
