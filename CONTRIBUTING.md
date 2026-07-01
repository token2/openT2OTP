# Contributing

Thanks for your interest. This is an open reference for a TOTP hardware token —
contributions that improve correctness, portability, and documentation are very
welcome.

## Ground rules

- **Keep it verifiable.** Cryptographic code must pass its known-answer test.
  `make test`, `make kat`, and `make totp` must all stay green.
- **Portable C.** The firmware core targets plain C99 with no host dependencies
  beyond the HAL. Keep MCU-specific code behind the HAL boundary.
- **Design your own housing.** Don't contribute enclosure artwork that copies an
  existing product's look and feel (see `hardware/CASING.md`).
- **Be honest about security.** Don't describe TOTP as phishing-resistant; it
  isn't (see `docs/SECURITY.md`).

## Good first contributions

- **Additional LCD glass pin-maps** — a `hal_slcd_set_digit()` for a specific,
  easy-to-source segment glass.
- **A routed KiCad PCB** for the reference board (`hardware/kicad/`), starting from
  your NFC front-end vendor's reference antenna.
- **Antenna coil geometry + measured inductance** for the reference outline.
- **Off-the-shelf component reports.** We ordered parts made specifically for this
  core, but standard components should work too. If you build on an off-the-shelf
  MCU/LCD/NFC part and hit an incompatibility — or confirm one works — please open
  an issue or PR. These reports are genuinely valued and help us keep the design
  broadly buildable.

## Workflow

1. Fork and branch from `main`.
2. Build and run the tests:
   ```
   cd firmware && make test && make kat && make totp
   ```
3. Keep changes focused; one topic per PR.
4. Open a PR describing what changed and how you verified it. For firmware, note
   which tests you ran; for hardware, note how you validated (bench measurement,
   simulation, etc).

## Code style

- C99, 4-space indent, no tabs in sources.
- No dynamic allocation in the firmware core.
- Comment the *why*, not the *what*.

## Reporting security issues

See [`SECURITY.md`](SECURITY.md) — please disclose privately first.
