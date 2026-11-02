# Changelog

All notable documentation changes to `RIoT2.Ard.WiegandI2C`.

## [Unreleased]

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
