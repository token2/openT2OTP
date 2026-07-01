# TOTP token — LCD, wiring, BOM & PCB design

This extends the device-side firmware with the **display and board design** for a
self-contained programmable TOTP token: a device that shows a 6-digit code on a
segment LCD, keeps time on a coin cell for 5–8 years (model-dependent), and is
provisioned over NFC by any host implementing the Token2 programmable-token
protocol.

The whole approach is modeled on how commercial tokens (and the open-source
Sensor Watch) actually do it: a **raw segment-LCD glass driven directly by an
MCU that has a built-in LCD controller** — no separate display driver, always-on
at microamps, years on one cell.

## 0. Form factor: keyfob dongle (main) vs card (variation)

This matches the real Token2 line, which ships both a **keyfob dongle** (the
C302 style) and a **card** token. Electrically they are the *same design* — 6-digit
segment LCD, one power/show button, coin-cell-class power, NFC programming with no
port. The only differences are mechanical: antenna size, battery, and rigidity.

**Main target: the keyfob dongle (C302-style).** It's the easier and more
prototype-friendly build:
- A normal **rigid FR-4 board (~1.6 mm)** with vertical room for a proper tactile
  button and a **user-replaceable CR2032 holder** on the back.
- A **larger perimeter antenna loop** → better NFC read range and easier tuning.
- A molded two-part case with a key-ring tab.

**Variation: the card.** Same schematic, harder mechanicals:
- Card body is ~0.8 mm, so it needs a **flex or ultra-thin rigid-flex PCB** and a
  **printed/embedded antenna** in a smaller area.
- Powered by a **soldered thin coin/pouch cell** (e.g. a thin CR-class cell), not
  a holder — which is why production card tokens are **sealed and disposable** when
  the battery is exhausted.
- Assembly is typically laminated rather than a snap case.

Everything below (MCU, LCD, firmware, BOM) is shared; the two "PCB design"
subsections at the end call out what changes for the card.

---

## 1. Why a segment LCD (not OLED / TFT / LED)

| Option | Idle current | Always-on | Fits a card | Verdict |
| --- | --- | --- | --- | --- |
| **Segment LCD (transflective TN)** | ~1–5 µA | yes | yes | **Chosen** |
| OLED | mA-class when lit | no | thick | too power-hungry |
| TFT | tens of mA + backlight | no | thick | far too power-hungry |
| LED 7-seg | mA even dimmed | no | yes | drains a coin cell in days |

A TOTP token must hold the display readable while sipping current from a CR2032
(~225 mAh). Only a reflective/transflective segment LCD does that. The trade-off
is that a segment LCD can only show fixed shapes — which is fine, we only need
digits 0–9.

At a segment-LCD idle draw of ~1–5 µA, a ~225 mAh CR2032 lasts on the order of
**5–8 years** (225 mAh / a few µA ≈ tens of thousands of hours), which is the
service life real programmable TOTP tokens quote. Exact life depends on the
model: display duty, how often the code is shown, the RTC/MCU sleep current, and
self-discharge of the cell. Lower-power sleep modes and a smaller display push
toward the upper end; a larger display or frequent button use toward the lower.

---

## 2. How the LCD wires to the MCU

A multiplexed segment LCD is a grid of **COM (backplane)** lines crossing
**SEG (segment)** lines. Each bar of each digit is one pixel that lights when its
COM/SEG intersection is driven. The MCU's SLCD peripheral generates the AC
drive waveforms (bias voltages, frame-inverted so there's **no net DC** — DC
permanently damages LCD glass).

For a **6-digit, 7-segment** display = 42 segments. With a common **1/4-duty**
glass:
- 4 COM lines
- ⌈42 / 4⌉ = **11 SEG lines**
- **15 MCU pins total**, all from the SLCD peripheral.

The glass does **not** solder to the board. It connects by either:
- a **zebra strip** (elastomeric connector) pressed between glass and PCB pads by
  the housing, or
- a **heat-seal FPC tail** bonded to the board.

Wiring summary:

```
SAM L22 SLCD pins  ──►  COM0..COM3      ─┐
                   ──►  SEG0..SEG10     ─┴─► zebra strip ─► LCD glass
LCD bias caps      ──►  CAPL/CAPH, VLCD reservoir caps (per datasheet)
```

The SAM L22 has an internal charge pump / bias generator, so the only external
LCD parts are a few small bias reservoir capacitors (the datasheet's LCD section
lists the exact values). No contrast pot, no driver IC.

**Segment-to-pin mapping** is defined by the *glass* datasheet: it gives a pinout
table listing, for every segment, which COM and SEG pin it sits on. You transcribe
that table into `hal_slcd_set_digit()` (in the firmware). The portable font and
digit logic live in `firmware/src/slcd.c`; only the COM/SEG bit placement is board-specific.

---

## 3. Recommended hardware

### 3.1 MCU — the linchpin

**Microchip SAM L22 (ATSAML22N18A)** — 32-bit Cortex-M0+, and critically it
integrates:
- a **segment-LCD controller** (up to 48 SLCD pins / 320 segments — far more than
  our 15),
- a **32-bit RTC** (the TOTP time base),
- **AES**, ample flash/SRAM,
- ultra-low-power sleep modes measured in **µA**.

This is the same class of part (SAM L22) used in ultra-low-power always-on
segment-LCD designs that run for years on a coin cell. It collapses MCU + LCD
driver + RTC into one chip, which is exactly what you want in a token.

> SM4 note (from the protocol work): the SAM L22's AES block does **not** do SM4,
> so the SM4 used by the Token2 protocol runs in software (`firmware/src/sm4.c`). It's
> cheap on an M0+ and only runs during the rare NFC programming session, not on
> every display refresh.

### 3.2 Display

A **6-digit 7-segment transflective TN glass**, custom or catalog. Suppliers:
Good Display (e.g. GDC-series TN transflective segment glass), Crystalfontz,
Truly, Varitronix. For a first prototype, a catalog 6-digit static/multiplexed
TN glass with a zebra connector is the fastest path; for production you order a
custom glass with your exact digit layout and any icons (battery, lock).

### 3.3 NFC front-end

The host talks ISO 14443-4 (NFC Type-4). Two routes:
- **Discrete NFC controller in card-emulation:** NXP PN7160 / PN5180, or ST
  ST25R3916. These present a proper ISO-DEP endpoint; you feed each received APDU
  straight into `token2_process_apdu()`.
- **Passive NFC tag front-end with I²C:** ST ST25DV — simpler and lower power,
  but you drive the APDU exchange through its mailbox/energy-harvest path.

For a battery token that must answer live APDUs, a controller like the
**ST25R3916** or **PN7160** is the clean choice.

### 3.4 Optional secure element (seed vault)

The seed should live in tamper-resistant storage. Add **NXP SE050** or
**Microchip ATECC608B** over I²C purely to hold the seed and serve the TRNG;
SM4 and APDU logic stay on the MCU. (These provide AES/ECC, not SM4 — that's
fine, they're a vault, not the protocol engine.)

**This is what makes "the seed can't be extracted" true.** The wire protocol is
already write-only (no read-seed command), but on a bare MCU the seed still sits in
on-die flash and is recoverable by a determined attacker. Put it in an SE and it
never leaves the SE's protected store. Best practice: inject the seed into the SE
**marked non-exportable** and run the HMAC **inside** the SE, so the seed is never
present in MCU RAM — even compromised firmware can't read it. Our production tokens
do exactly this. See [`docs/SECURITY.md`](../docs/SECURITY.md) → "Seed protection"
for the storage-vs-extraction table (Pico/plain flash = assume extractable; SE with
non-exportable key = not extractable). Omit the SE for a hobby/demo build; include
it for anything resembling a product.

### 3.5 Timekeeping

The SAM L22 RTC runs from a **32.768 kHz watch crystal** (e.g. Epson FC-135 or
Abracon ABS07). This sets TOTP accuracy — use a crystal with a low ppm rating so
codes don't drift between programmings.

### 3.6 Power

**Keyfob (main):** **CR2032** (3 V, ~225 mAh) in a **back-side replaceable
holder**, with decoupling caps. At a few µA average the token runs about **5–8
years** on one cell (model-dependent — see §1), and the user can swap the cell.

**Card (variation):** a **soldered thin coin/pouch cell** in the ~0.8 mm budget;
no holder, so the card is sealed and replaced (not serviced) at end of life.

---

## 4. Bill of materials (single prototype)

Reference prices are indicative single-unit/small-qty (USD) and will drop sharply
at volume. Always confirm live pricing and stock at a distributor.

| # | Ref | Part | Function | Example MPN | ~Qty | ~Unit $ |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | U1 | MCU w/ SLCD+RTC | brains, LCD drive, timekeeping | ATSAML22N18A | 1 | 4–6 |
| 2 | DS1 | 6-digit 7-seg TN LCD glass | display | Good Display GDC-series (or custom) | 1 | 2–5 |
| 3 | — | Zebra strip / heat-seal | glass-to-PCB contact | elastomeric connector | 1 | 0.3–1 |
| 4 | U2 | NFC controller | ISO 14443-4 card emulation | ST ST25R3916 / NXP PN7160 | 1 | 2–4 |
| 5 | U3 | Secure element (optional) | seed vault + TRNG | NXP SE050 / Microchip ATECC608B | 1 | 0.5–2 |
| 6 | Y1 | 32.768 kHz crystal | RTC time base | Epson FC-135 / Abracon ABS07 | 1 | 0.3–0.6 |
| 7 | BT1 | CR2032 holder (keyfob) | replaceable coin-cell socket | Keystone 3002 (or SMD equiv) | 1 | 0.3–0.7 |
| 8 | — | CR2032 cell | power (keyfob) / thin cell for card | any | 1 | 0.3 |
| 9 | L1 | NFC antenna | PCB copper loop (no part) | on-board etch | — | 0 |
| 10 | C1,C2 | Antenna tuning caps | match coil to 13.56 MHz | C0G/NP0, values from tuning | 2 | 0.02 |
| 11 | C3–C6 | LCD bias reservoir caps | SLCD charge-pump | X7R, per L22 datasheet | ~4 | 0.02 |
| 12 | C7–C10 | Decoupling caps | VDD rails | 100 nF / 1 µF X7R | ~4 | 0.02 |
| 13 | SW1 | Power/show button | wake + display code | tactile SMD (keyfob) / dome (card) | 1 | 0.1 |
| 14 | J1 | SWD pads/header | program & debug | 2x5 1.27 mm or test pads | 1 | 0.2 |
| 15 | — | PCB | keyfob: 2-layer FR-4 ~1.6 mm · card: flex/rigid-flex ~0.8 mm | fab | 1 | 1–3 (proto) |

**Indicative BOM total (prototype, incl. optional SE):** ~$13–28 depending on the
display and NFC controller. At volume the electronics fall well under $10.

> **Sourcing note.** We **ordered components manufactured specifically for this
> code to run on** — that's what our production tokens use, and they are proven.
> Those exact parts are **not always easily sourceable** (some are factory-tailored
> equivalents of a mainstream component). Our **theoretical and bench research
> suggests standard off-the-shelf components should work as well**, and the MPNs
> listed above are those mainstream, buy-anywhere equivalents so you can reproduce
> the design without our supply chain. They are examples, not requirements: any MCU
> with an integrated segment-LCD controller + RTC + enough flash, any 13.56 MHz
> front-end, and any transflective 6-digit glass should do.
>
> If small changes turn out to be needed to run on a particular off-the-shelf part,
> **we'll update the design** — and **community updates, and even just reports of
> such incompatibilities, are very welcome** (open an issue or PR).

---

## 5. PCB design guidance

**Main: keyfob dongle.** Rigid 2-layer FR-4, ~1.6 mm — the extra thickness gives
vertical room for a tactile button, a CR2032 holder on the back, and a two-part
molded case with a key-ring tab. The floorplan diagram above shows the placement:
LCD at one end, power button at the other, MCU + crystal central, antenna around
the perimeter. The rules that actually matter:

1. **NFC antenna = perimeter loop.** Run 3–5 turns of copper around the board
   edge on both layers, stitched with vias at the corners, to maximize the
   enclosed area (coupling scales with loop area). The keyfob's larger outline
   gives a bigger loop and better read range than the card.
2. **Antenna keep-out is sacred.** No ground pour, no battery, no large copper
   *inside* the loop — metal detunes the coil and kills read range. The coin cell
   holder goes on the back, offset outside the antenna footprint.
3. **Tune to 13.56 MHz.** The coil inductance plus C1/C2 forms the resonant tank.
   Measure the coil, compute C, then trim on the bench with a VNA / reader
   read-range test.
4. **No copper under the LCD.** Keep the zebra contact zone clean; a pour under
   the glass can cause uneven contrast and contact issues.
5. **Crystal placement.** Put Y1 close to the MCU RTC pins with a local ground
   guard; keep noisy NFC traces away from it so time doesn't jitter.
6. **MCU central**, SLCD pins fanning to the LCD pads, NFC pins to the antenna
   edge, I²C to the secure element, SWD pads accessible for programming.
7. **Decoupling** close to every VDD pin; the SLCD bias caps per the L22 datasheet
   LCD section.

**Variation: card.** Same schematic and rules, but the ~0.8 mm body forces:
- a **flex or rigid-flex PCB** instead of 1.6 mm FR-4;
- a **printed/etched antenna** in a smaller area — expect lower read range, so
  budget more care in tuning;
- a **soldered thin cell** and a **dome/membrane button** instead of a holder and
  tactile switch;
- **lamination** rather than a snap case, making the unit sealed/disposable.
Keep the same block placement, just flattened into the thinner stack-up.

**Tools:** KiCad (free) is ideal — its footprint editor handles the custom
antenna coil and LCD zebra pads. Many NFC-controller vendors publish reference
antenna designs and tuning spreadsheets; start from those rather than computing
the coil from scratch.

---

## 6. Firmware for the display path

Two new modules sit on top of the protocol core:

- **`firmware/src/totp.c`** — RFC 6238 TOTP over the stored seed (HMAC-SHA1). Verified
  against the RFC 6238 published test vectors (T=59 → `287082` for 6 digits,
  and the 8-digit `94287082` / `07081804` vectors). This turns the seed written
  into the code shown on screen (the seed written by any conformant host).
- **`firmware/src/slcd.c`** — 7-segment font + digit placement. `slcd_show_code(code, 6)`
  renders a zero-padded 6-digit code; `hal_slcd_set_digit()` (which you fill in
  from the glass pinout) writes the COM/SEG bits into the SLCD frame buffer.

Typical main loop on the MCU:

```
on wake (RTC tick or button):
    t     = hal_rtc_get()
    code  = totp_now(seed, seed_len, t, step_seconds, 6)
    slcd_show_code(code, 6)
    stay awake briefly, then slcd_blank() and deep-sleep
on NFC field detect:
    run token2_process_apdu() per APDU  (host programming session)
```

Build/verify the display math on the host:

```
cd firmware
make totp      # RFC 6238 vectors -> PASS
```

---

## 7. What's proven vs. what you finish on hardware

**Proven in this repo (host-verified):**
- Full Token2 wire protocol (auth, MAC, seed & config writes) — `make test`
- SM4 vs GM/T 0002 KAT — `make kat`
- TOTP vs RFC 6238 KAT — `make totp`

**You complete on the target:**
- The four HAL functions (RTC, NVM, RNG, constant-time compare)
- `hal_slcd_set_digit()` from your specific glass pinout
- NFC controller driver → APDU hand-off
- Antenna tuning on the bench

Everything above the HAL line is portable C that already works; the
board-specific glue is small and well-scoped.

---

## 8. Design files in this folder

- `bom.csv` — machine-readable bill of materials (importable into sourcing tools).
- `BOM.md` — the same **suggested BOM** as a readable table with reference
  designators; `kicad/bom-table.png` is a rendered version (via `kicad/draw_bom.py`).
  Example parts, not a locked BOM — substitute equivalents freely.
- `PINMAP.md` — MCU pin → net → component assignment.
- `kicad/openT2OTP.kicad_sch` — self-contained KiCad 7 connectivity schematic
  (symbols embedded; opens with no external libraries).
- `kicad/openT2OTP.net` — KiCad netlist, the electrical source of truth (27 nets).
- `kicad/openT2OTP.kicad_pcb` — PCB **outline + mounting holes + placement**, sized
  48×24 mm to fit the OT1 enclosure (4× M2 at ±19.2/±7.3 mm). Placement only — not
  routed; see `kicad/README.md`.
- `kicad/gen_schematic.py` — regenerates the schematic + netlist from the net table.
- `kicad/gen_pcb.py` / `kicad/gen_gerber.py` — regenerate the PCB placement and the
  mechanical (outline + drill) Gerbers in `kicad/gerbers/`.
- `kicad/placement.png` — a **suggested placement diagram** (illustration, not a
  manufactured PCB and not a routed board). See `kicad/README.md` for what it is and
  isn't.
- `antenna/coil.md` — reference coil geometry for the keyfob outline.
- `antenna/tune.py` — 13.56 MHz tuning-capacitor calculator.
- `CASING.md` — enclosure / industrial-design notes (deliberately our own).

> The two `.py` files here are **host-side design tools** — a schematic generator
> and an antenna calculator you run on your PC. They are not part of the device:
> the **firmware is C only** and contains no Python.

These are a **connectivity design**, not a routed board: correct parts and nets,
ready for you to add real footprints and lay out copper. See `kicad/README.md`.
