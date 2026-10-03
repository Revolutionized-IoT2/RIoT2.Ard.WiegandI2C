# AGENTS.md — RIoT2.Ard.WiegandI2C

Applies to: this repository. Read the platform guide first:
[.github/AGENTS.md](https://github.com/Revolutionized-IoT2/.github/blob/main/AGENTS.md). It covers the
workspace map, platform-wide rules and the documentation rules. In the local workspace, every
`https://github.com/Revolutionized-IoT2/<Repo>/blob/main/<path>` link is the file
`C:\Src\RIoT2\<Repo>\<path>`; read the local file instead of fetching the URL.

## What this is

Standalone ATtiny85 Arduino sketch that decodes Wiegand26 reader frames and exposes the decoded
24-bit code as a three-byte I2C slave response. It is a planned hardware building block for RIoT2
firmware, but no ESP32 node currently integrates it.

## Commands

There is no PlatformIO project or CI workflow in this repository.

Build and flash with the Arduino IDE:

```powershell
# Open C:\Src\RIoT2\RIoT2.Ard.WiegandI2C\WiegandI2C.ino in the Arduino IDE.
# Select ATtiny85, 8 MHz internal clock, then use Upload Using Programmer.
```

Run host regressions from `C:\Src\RIoT2\RIoT2.Ard.Shared` when a C++14 compiler is available:

```powershell
python .\tests\test_firmware_p1.py
python .\tests\test_firmware_p2.py
```

- Build: Arduino IDE/ATtiny toolchain only; no `pio run` command exists here.
- Test: shared native tests compile the production USI driver and sketch callbacks with fake AVR
  registers.
- Run: no host runtime exists.
- Release: no CI workflow or git tags are present. Flash via ISP programmer for hardware use.

## Layout

| Path | Contents |
|---|---|
| `WiegandI2C.ino` | ATtiny85 setup, Wiegand ISR, Timer1 timeout, I2C request callback and local diagnostic |
| `TinyWireS.h`, `TinyWireS.cpp` | USI I2C slave wrapper used by the sketch |
| `usiTwiSlave.h`, `usiTwiSlave.c` | Low-level AVR USI/TWI slave driver |
| `README.md` | Human-facing wiring, protocol and build notes |

## Contracts consumed here

This repository does not currently implement platform MQTT, HTTP or configuration contracts. Future
integration should expose it through `RIoT2.Ard.Shared` as an `IPeripheral` using:

- [configuration.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/configuration.md)
  for the peripheral template.
- [mqtt-topics.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/mqtt-topics.md)
  for the report emitted by the ESP32 host node.

## Rules

- Keep this sketch standalone until a `WiegandI2CPeripheral` is added to `RIoT2.Ard.Shared`.
- Do not add ESP32 or PlatformIO dependencies to this repository.
- Keep I2C address `0x26` and the three-byte response stable unless the hub feature/design is
  updated with a migration.
- Preserve acquisition safety: malformed or in-progress frames must not overwrite an unread
  completed code.
- Do not remove parity validation for Wiegand26 frames.
- Hardware claims must be checked against `WiegandI2C.ino`, not copied from old notes.

## Pitfalls

- The sketch consumes the pending code when `requestEvent()` queues the response, not when the I2C
  master finishes reading. An aborted read loses that code.
- `TinyWireS` has no receive handler wired here; the slave cannot be configured over I2C.
- ATtiny85 has no hardware I2C/TWI peripheral; the bundled USI driver requires external pull-ups.
- `droppedFrameCount` is only a local diagnostic exposed by `wiegandDroppedFrames()`; it is not
  part of the three-byte I2C response.
- This bridge is not yet wired into M5Core2 or M5Dial firmware.

## Related work

- Architecture proposal [A8](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/target.md#a8-share-a-firmware-noderuntime):
  shared firmware `NodeRuntime` and future Wiegand peripheral.
- Plan [M5](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/plans/m05-firmware-node-runtime.md):
  add `WiegandI2CPeripheral : IPeripheral` to shared firmware.
- [Feature ideas](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/features.md#firmware):
  integrate `WiegandI2C` as an I2C `0x26` peripheral with an optional IRQ pin.
