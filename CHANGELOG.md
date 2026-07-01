# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/).

## [2.4.0] — 2026-07-01

Readable suggested BOM table.

### Added
- **`hardware/BOM.md`** — the bill of materials as a readable table with reference
  designators (Ref, Qty, Component, Value/Part, Example MPN, Package, Notes), clearly
  framed as a **suggested / reference setup, not a locked BOM**. Reference designators
  match the netlist; the BOM adds the extra decoupling caps, the cell, and the zebra
  connector a real build needs.
- **`hardware/kicad/bom-table.png`** — a styled rendered version of the table, and
  **`kicad/draw_bom.py`** to regenerate it from `bom.csv`.

## [2.3.0] — 2026-07-01

Clear placement diagram, honestly labelled.

### Added
- **`kicad/placement.png`** — a readable component-placement diagram with realistic
  part shapes (QFN packages, round coin cell, LCD with segment digits, crystal can,
  SWD pads), front/back, matched to the reference board's arrangement.
- **`kicad/draw_placement.py`** — renders it from the `.kicad_pcb`.

### Documented
- The kicad README now explains plainly that `placement.png` is a **suggested
  placement diagram — an illustration, not a manufactured PCB and not a routed
  board** — and why a Gerber never shows components (Gerbers describe only the bare
  board; a populated look requires a 3D render of a routed board). Same note added to
  `hardware/HARDWARE.md`.

## [2.2.0] — 2026-07-01

PCB placement revised to match the real production board.

### Changed
- **Placement now matches photographs of the actual board:** battery is an
  edge-mounted coin-cell clip at one end (not a centered holder); the IC cluster
  (MCU, NFC front-end, secure element, 32.768 kHz crystal can) sits at the other
  end; the "show" button is a through-hole dome on the front beside the LCD; the
  LCD is a chip-on-glass module filling ~60% of the front face. Added the board's
  **edge retention notches** to both the `.kicad_pcb` and the outline Gerber.
- Added an automatic overlap-resolution pass to `gen_pcb.py` so passive placement
  stays courtyard-clean while preserving the photo-matched big-part arrangement.
- Component values remain generic (real MPNs intentionally not published).

## [2.1.0] — 2026-07-01

PCB outline, placement, and mechanical Gerbers fitted to the OT1 enclosure.

### Added
- **`kicad/openT2OTP.kicad_pcb`** — a KiCad 7 board with the **outline, mounting
  holes, and full component placement**, sized to the real OT1 enclosure geometry
  (measured from the supplied STEP: 48×24 mm board, 4× M2 mounting holes at
  ±19.2/±7.3 mm boss positions). LCD on the front face; MCU, NFC, secure element,
  crystal, coin cell, SWD and passives on the back. Placement is overlap-checked
  and confined to the board.
- **`kicad/gen_pcb.py`** — the PCB generator (geometry derived from the enclosure).
- **`kicad/gen_gerber.py`** + **`kicad/gerbers/`** — a fabricable **board-outline
  Gerber** and Excellon **drill** file (validated: closed 48×24 mm profile, 4 holes).
- **`kicad/placement.png`** — front/back placement preview.

### Honest scope
- The board is **placement + mechanical outline only — not routed.** No copper
  traces, ground pour, or tuned antenna yet; the perimeter loop is placeholder
  geometry. The outline + drill Gerbers are fab-ready; the copper Gerbers are not,
  and must follow interactive routing + DRC in KiCad. Documented in
  `kicad/README.md`. This closes part of the long-standing "no routed board" gap —
  the mechanical/placement part — while being explicit about what routing remains.

## [2.0.0] — 2026-07-01

Renamed to **openT2OTP** and refreshed the brand.

### Changed
- **Project renamed openTOTP → openT2OTP** (T2 = Token2), because "openTOTP" was
  already taken. Applied across all file contents, the KiCad schematic/netlist
  filenames (`openT2OTP.kicad_sch`, `openT2OTP.net`), the source headers, and docs.
- **Logo & icon updated** (`docs/logo.svg`, `docs/icon.svg`): the countdown-ring
  token now carries a **T2 monogram** tying it to Token2, with a "TOKEN2" tagline.
  (Original mark, not Token2's trademarked logo — swap in the official icon if
  desired.)

### Note
- The only Python in the repo is two **hardware design tools**
  (`hardware/kicad/gen_schematic.py`, `hardware/antenna/tune.py`) — a schematic
  generator and an antenna-tuning calculator. **The firmware is C only**; no Python
  runs on the device or in the firmware build.

## [1.6.0] — 2026-07-01

Licensing headers and a second seed-security pass.

### Added
- **SPDX license headers on every source file.** All firmware `.c`/`.h` files carry
  `SPDX-License-Identifier: MIT` and `Copyright (c) 2020-2026 Token2 Sarl`; the
  hardware Python tools carry the CERN-OHL-P header. LICENSE and LICENSE-HARDWARE
  holder updated from the placeholder to **Token2 Sarl**.

### Changed / Fixed
- **Seed hygiene (defensive):** `cmd_seed()` now zeroizes the stored seed buffer
  before writing a new seed (no stale tail bytes from a longer previous seed) and
  scrubs the decrypted-seed scratch buffer off the stack after use.
- **Seed-protection docs expanded** after a second audit: documented that the
  display-path accessor copies the seed into MCU RAM (fine for a plain-MCU build,
  but for a secure-element build the HMAC must be computed *inside* the SE so the
  seed never enters RAM), plus the zeroization behavior. The accessor in `token2.c`
  now carries this SE caveat inline. Re-confirmed: the wire protocol has no
  seed-read path and no response emits seed bytes.

## [1.5.0] — 2026-07-01

Seed-protection documentation, logo, and naming note.

### Added
- **Seed protection section** in `docs/SECURITY.md` making the two guarantees
  explicit and separate: (1) the seed is **write-only over the wire** — the
  protocol has no read-seed command and no response ever contains seed bytes
  (a hard property of the dispatcher; verified by inspection); (2) **extraction
  resistance depends on where the seed is stored**, with a concrete table:
  Pico / plain MCU flash = assume extractable; debug-locked MCU = partial; discrete
  secure element (SE050 / ATECC608) = not extractable, especially with a
  non-exportable key and HMAC computed inside the SE. `hardware/HARDWARE.md`
  strengthened to match.
- **Project logo** (`docs/logo.svg`) and icon (`docs/icon.svg`): a TOTP countdown
  ring around a 7-segment digit on a keyfob silhouette. Added to the README header.

### Changed
- **Naming note** added to the README: "openT2OTP" is the name chosen for the
  open-source release; the firmware was developed internally under a different
  codename. The name is new; the core is not.

## [1.4.0] — 2026-07-01

Real production provenance, and the SHA-256 path made real.

### Added
- **HMAC-SHA256 (`algo=2`) fully implemented** in the TOTP path. Previously the
  display path only did SHA1 and silently ignored the configured algorithm; the
  config already stored `hmac_algo`, but generation didn't honor it. Added a
  SHA-256 core and `totp_now_algo(...)`; `totp_now()` remains a SHA1 wrapper.
  Verified against the RFC 6238 Appendix B SHA-256 vectors (46119246 / 68084774 /
  90698825) alongside the existing SHA-1 vectors.

### Changed
- **Provenance corrected across the docs.** This is documented as the **actual
  firmware core our tokens run**, now open-sourced — not a clean-room
  reconstruction. Added project history: core **finalized in 2020**, most recent
  change in **2025** (removal of the wrong-key lockout), and it is exactly what
  current hardware runs. Updated the root README, `firmware/README.md`, and
  `docs/SECURITY.md` (lockout history).
- **Hardware sourcing note reframed:** components were **ordered specifically for
  this core**; research indicates off-the-shelf parts should also work; we'll
  update for small changes, and community reports of incompatibilities are welcome.
  Updated `hardware/HARDWARE.md`, the README, and `CONTRIBUTING.md`.

## [1.3.0] — 2026-07-01

Removed the Raspberry Pi Pico port.

An earlier revision added a Pico + Waveshare-LCD dev-board port as a
zero-sourcing way to run the firmware. It was removed: re-implementing the
display/transport on a Pico (and the MicroPython smoke test that came with it)
was not reproducible in the same sense as the rest of the project — a Pico has
no secure element and no battery, so it could never be the offline token this
design is about, and it muddied what the repo demonstrates. The project is
back to its two intended parts: the verified C firmware and the secure-element
hardware reference.

### Removed
- `firmware/ports/pico/` (Pico HAL, pixel display, USB-serial transport, optional
  USB-keyboard typing, MicroPython smoke test) and the `pico-render` / `pico-kbd`
  make targets and CI steps.

### Kept
- `make provision` — the full challenge→auth→config→seed flow verified against the
  RFC 6238 vector. It is transport-agnostic (not Pico-specific) and remains a core
  test.
- The **sourcing note**: production parts are proven but not always easily
  sourceable (some are factory-tailored equivalents); the BOM lists mainstream
  buy-anywhere equivalents.

## [1.1.0] — 2026-07-01

Correctness pass and hardware design files.

### Fixed
- **Seed length cap.** `cmd_seed()` rejected seeds whose ciphertext exceeded 48
  bytes, which wrongly refused seeds of 48–63 bytes (a 63-byte seed frames to a
  64-byte ciphertext). Cap corrected to 64. All seed lengths 1..63, in both the
  general and 32-byte-special framings, now round-trip exactly (new `make seed`).

### Added
- `make seed` — seed framing round-trip test across lengths 1..63 and both forms.
- `make slcd` — segment-LCD driver test with an ASCII render of the digits, plus
  host stubs for the three `hal_slcd_*` hooks (reference for the display port).
- `make all` — run every verification target.
- Hardware design files: machine-readable `bom.csv`, KiCad connectivity schematic
  and netlist, `PINMAP.md`, and an antenna coil/tuning reference.

### Changed
- Battery life corrected throughout to **5–8 years (model-dependent)** with the
  supporting current/capacity math, replacing the earlier ~1-year figure.
- Framing reworded to lead with the **wire protocol**: the firmware implements the
  documented protocol and interoperates with any conformant host; `token2_config.py`
  is cited as one such open host rather than the thing being "answered".
- Clarified `display_timeout` wire values (0..3) vs the host `--sleep` argument
  indexing (1..4).

## [1.0.0] — 2026-07-01

Initial public release of the production firmware core (finalized 2020).

### Added
- Device-side firmware implementing the Token2 single-profile TOTP wire protocol
  (get-info, SM4 challenge/response auth, MAC-protected seed and config writes).
- SM4 (GB/T 32907) implementation with GM/T 0002 known-answer test.
- RFC 6238 TOTP generation (HMAC-SHA1) with RFC test-vector verification.
- Segment-LCD display driver with a portable 7-segment font and a board-specific
  segment-map hook.
- NFC transport shim for secure-element applet or MCU ISO-DEP integration.
- Hardware reference: LCD wiring, costed BOM, PCB floorplan, antenna and casing
  notes. Keyfob-dongle main form factor, card documented as a variation.
- Documentation: protocol reference, security/threat model (TOTP vs FIDO2),
  porting guide.
- CI running all three verification targets on every change.

### Notes
- TOTP is not phishing-resistant; use FIDO2 / WebAuthn where phishing resistance
  is required. See `docs/SECURITY.md`.
- This initial release shipped the SHA-1 display path; full HMAC-SHA256 (config
  `algo=2`) was completed later — see 1.4.0.
