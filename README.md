# RIoT2.Ard.WiegandI2C

An ATtiny85 sketch that decodes a Wiegand26 card reader and exposes the decoded card code over
I2C. It acts as a small, standalone protocol bridge/level-shim: most microcontrollers (including
the ESP32-based [RIoT2.Ard.M5Dial.Node](../RIoT2.Ard.M5Dial.Node) /
[RIoT2.Ard.M5Core2.Node](../RIoT2.Ard.M5Core2.Node) nodes) have no native Wiegand peripheral and
would otherwise need two dedicated interrupt-capable GPIOs and precise inter-bit timing just to
read a card swipe; this offloads that to a cheap, dedicated chip and hands the result to the host
as three plain I2C register-read bytes.

This project is **not currently wired into any RIoT2 node firmware** — it's a hardware building
block. A node's `GpioPeripheral`-style I2C master code would need to poll it (see **I2C protocol**
below) and publish the result as a `Report`; that integration doesn't exist yet.

## Hardware

- **MCU:** ATtiny85 (or any ATtiny with USI hardware and enough flash — this uses ~2KB).
- **Wiegand reader:** any standard 26-bit Wiegand card/badge reader (D0/D1 open-collector data
  lines + power).
- **I2C:** the ATtiny85 has no hardware I2C peripheral; this uses its USI (Universal Serial
  Interface) in I2C-slave mode via the bundled `TinyWireS`/`usiTwiSlave` driver (adapted from Don
  Blake's AVR312 application note code), since the standard Arduino `Wire` library only supports
  I2C on parts with real TWI hardware.

### Pin map (ATtiny85, `PORTB`)

| Signal | Pin | Notes |
| --- | --- | --- |
| Wiegand D0 | `PB3` | Input, pin-change interrupt |
| Wiegand D1 | `PB4` | Input, pin-change interrupt |
| Data-ready IRQ (output) | `PB1` | Driven high once a valid 26-bit code has been decoded; goes low again once the host reads it (or immediately, if `requestEvent()` finds nothing new) |
| I2C SDA / SCL | `PB0` / `PB2` | USI-driven; **requires external pull-ups** (more so than the AVR `Wire` library, per `TinyWireS.h`'s own note) |

The pin assignments are `#define`s at the top of [WiegandI2C.ino](WiegandI2C.ino)
(`WGD_D0`/`WGD_D1`/`WGD_IRQ`), parameterized so the same logic could be retargeted to another AVR.

## I2C protocol

- **Slave address:** `0x26` (`I2C_SLAVE_ADDR` in [WiegandI2C.ino](WiegandI2C.ino)).
- **Read (3 bytes):** the host issues a plain I2C read (no register/command byte needed —
  `requestEvent()` is the only handler wired up). The reply is the low 3 bytes of the decoded
  Wiegand26 value (parity bits stripped), **most-significant byte first**:

  | Byte | Content |
  | --- | --- |
  | 0 | bits 23–16 |
  | 1 | bits 15–8 |
  | 2 | bits 7–0 |

  If no new code has been decoded since the last read, all three bytes are `0`. After a
  successful read the internal buffer is cleared and the IRQ line is dropped, so each card swipe
  is only reported once.
- **Short/aborted reads:** every addressed read starts a fresh response. Unread bytes from
  an earlier response are discarded before `onRequest` runs, including after a repeated START.
  Read all three bytes in one transaction; separate one-byte reads do not resume a response.
  The sketch still consumes the pending code when the request callback queues it, not when
  the master finishes reading, so an aborted read does not preserve that code for retry.
- **Transmit overflow:** `TinyWireS.send(byte)` and `usiTwiTransmitByte(byte)` return `true`
  when the byte is queued, or `false` when the TX buffer is full. Rejected bytes do not change
  the queued response and never block the interrupt handler. Callers producing variable-sized
  responses must check this result. The normal three-byte callback fits the default 16-byte buffer.
- **IRQ pin (`PB1`):** optional fast-path signal — poll it (or wire it to a host GPIO interrupt)
  instead of continuously polling over I2C. It goes high exactly when a fresh code is ready and
  low again once the host has read it.
- **Decoding:** only valid 26-bit Wiegand frames are accepted; the pin-change ISR shifts bits
  into a buffer as they arrive, a hardware timer (`TIMER1`) detects end-of-frame via a timeout,
  and any frame that doesn't end up exactly 26 bits or fails the standard Wiegand26 even/odd
  parity checks is silently discarded (`ISR(TIM1_OVF_vect)` in [WiegandI2C.ino](WiegandI2C.ino))
  rather than reported as a partial/garbage code.

## Build & flash

This is a plain Arduino `.ino` sketch, not a PlatformIO project (unlike the ESP32-based node
firmwares in this repo) — flash it with the Arduino IDE:

1. Install [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore) (or the classic
   [damellis/attiny](https://github.com/damellis/attiny) core) via Arduino's Boards Manager, so
   `Tools > Board` offers an ATtiny85 target.
2. Open [WiegandI2C.ino](WiegandI2C.ino) in the Arduino IDE — it will pick up
   [TinyWireS.h](TinyWireS.h)/[TinyWireS.cpp](TinyWireS.cpp) and
   [usiTwiSlave.h](usiTwiSlave.h)/[usiTwiSlave.c](usiTwiSlave.c) from the same folder
   automatically (Arduino treats every `.h`/`.cpp`/`.c` file next to the `.ino` as part of the
   same sketch).
3. Select **ATtiny85**, an 8MHz internal-clock variant (the Wiegand bit timing and I2C USI timing
   both assume this — no explicit `F_CPU`-based scaling exists in the code), and burn the
   bootloader/fuses once if using a fresh chip.
4. Flash via an ISP programmer (e.g. an Arduino-as-ISP, USBasp, or similar) — ATtiny85 has no USB
   or UART bootloader, so `Sketch > Upload Using Programmer` is required, not a plain serial
   upload.

## Host regression tests

With the sibling firmware repositories checked out, Python and a native C++14
compiler (a Visual Studio developer shell on Windows), run:

```powershell
python ..\RIoT2.Ard.Shared\tests\test_firmware_p1.py
python ..\RIoT2.Ard.Shared\tests\test_firmware_p2.py
```

The Wiegand test compiles the production USI driver with fake AVR registers,
the actual `send()` wrapper and sketch request callback. It exercises repeated
0/1/2/3-byte reads, NACK/repeated-START recovery, observable TX overflow, and the
unchanged normal three-byte/empty response. It uses no board or I²C hardware;
electrical timing and AVR integration still require a board build and hardware test.
The P2 regression feeds real sketch interrupt handlers with synthetic bit edges:
reading during a new frame preserves acquisition, invalid/oversized frames leave
the pending code intact, and a second valid completed frame increments the drop
diagnostic without replacing the first.

## Wiring summary

```
Wiegand reader D0  -> ATtiny85 PB3
Wiegand reader D1  -> ATtiny85 PB4
Wiegand reader GND -> common ground
Wiegand reader V+  -> reader's rated supply (commonly 12V; do NOT feed this into the ATtiny)

ATtiny85 PB0 (SDA) -> host SDA  (with pull-up to the shared logic-level rail)
ATtiny85 PB2 (SCL) -> host SCL  (with pull-up to the shared logic-level rail)
ATtiny85 PB1 (IRQ) -> host GPIO (optional, active-high "data ready" signal)
```

## Known limitations

- Only Wiegand26 is supported — other common frame lengths (e.g. Wiegand34/37) are rejected as
  timeouts, per the hardcoded `counter != 26` check. Wiegand26 parity is validated; bad-parity
  26-bit frames are rejected like malformed frames.
- No I2C `onReceive` handling is implemented (`TinyWireS.h`'s own TODO) — the slave only responds
  to reads, so there's no way to configure it (e.g. change the reported format) over the bus.
- Single pending completed code: **the first complete unread frame wins**. Acquisition
  uses separate storage, so incoming bits and malformed frames cannot alter the pending
  code or its ready IRQ. Additional complete frames while it remains unread are discarded,
  incrementing a saturating `droppedFrameCount`. `wiegandDroppedFrames()` provides an atomic
  local diagnostic snapshot; it is not exposed through the unchanged three-byte I2C protocol.
  Reading the pending code consumes it when the response is queued (including a later
  aborted read), but never resets an in-progress acquisition. No additional frame queue
  or parity validation is introduced.
