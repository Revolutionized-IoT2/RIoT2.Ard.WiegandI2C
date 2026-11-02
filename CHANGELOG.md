# Changelog

All notable documentation changes to `RIoT2.Ard.WiegandI2C`.

## [Unreleased]

- Docs: the M5Core2 and M5Dial firmware 0.6.0 read the module through the shared
  `WiegandI2CPeripheral` ([I42](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-3-4.md#i42-wiegand-peripheral-s)); wiring to the boards' Grove ports and the 3.3 V note. The sketch
  and its I2C protocol are unchanged.
- Build: `renovate.json` extends the shared Renovate preset ([I33](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-2.md#i33-supply-chain-s)). Renovate opens grouped
  update pull requests once the app is installed (MA7).
- Release: the firmware gets `firmware-<env>-<version>.spdx.json`, an SPDX SBOM of the PlatformIO
  platform.

## [0.1.0] - 2026-10-12

The first tagged release.

- Added a PlatformIO project (`platformio.ini`, ATtiny85 at 8 MHz) next to the Arduino sketch, CI,
  and a release workflow that attaches `firmware-attiny85-<tag>.hex` to a GitHub release.
- Documentation: `AGENTS.md` is the AI instruction file, `CLAUDE.md` imports it, and no version
  notes were present in the README.

## Earlier versions

See git commits; there are no release notes or git tags for earlier versions.
