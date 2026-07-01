# openT2OTP — Suggested Bill of Materials

> **This is a suggested / reference setup, not a locked BOM.** The parts below are
> *examples* that satisfy the design; substitute equivalents freely. The components
> used in our own production are proven but not always easily sourceable (some are
> factory-tailored equivalents), so this list points at mainstream, buy-anywhere parts.
> See the sourcing note in [`HARDWARE.md`](HARDWARE.md). Real production MPNs are not
> published. Reference designators match [`kicad/openT2OTP.net`](kicad/openT2OTP.net)
> (the netlist is a simplified connectivity model; this BOM adds the extra decoupling
> caps, the cell itself, and the zebra connector that a real build needs).

| Ref | Qty | Component | Value/Part | Example MPN | Package | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| U1 | 1 | MCU with SLCD + RTC | SAM L22 | ATSAML22N18A | TQFP64 | Cortex-M0+, integrated segment-LCD controller + RTC + AES; drives LCD directly |
| DS1 | 1 | Segment LCD glass | 6-digit 7-seg TN transflective | Good Display GDC-series (or custom) | Zebra/heat-seal | 42 segments; 1/4 duty, 1/3 bias; connects via zebra strip |
| CN1 | 1 | Zebra strip / heat-seal | Elastomeric connector | - | - | Glass-to-PCB contact under housing compression |
| U2 | 1 | NFC front-end | ISO 14443-4 card emulation | ST ST25R3916 (or NXP PN7160) | QFN32 | Presents ISO-DEP endpoint; feed APDUs to firmware |
| U3 | 1 | Secure element (optional) | Seed vault + TRNG | NXP SE050 (or Microchip ATECC608B) | SOIC8/UDFN | I2C; holds seed + serves RNG. SM4 stays on MCU |
| Y1 | 1 | Crystal | 32.768 kHz | Epson FC-135 (or Abracon ABS07) | SMD 3.2x1.5 | RTC time base; low-ppm for TOTP accuracy |
| BT1 | 1 | Coin-cell holder (keyfob) | CR2032 holder | Keystone 3002 | SMD | Replaceable cell on back; card variant uses soldered thin cell |
| BAT1 | 1 | Coin cell | CR2032 3V ~225mAh | - | Coin | Power source |
| L1 | 1 | NFC antenna | PCB copper loop | - | On-board etch | 3-5 turns around perimeter; no part - etched copper |
| C1 | 1 | Antenna tuning cap | C0G/NP0 (value from tuning) | - | 0402 | Resonant match to 13.56 MHz; trim on bench |
| C2 | 1 | Antenna tuning cap | C0G/NP0 (value from tuning) | - | 0402 | Resonant match to 13.56 MHz; trim on bench |
| C3 | 1 | LCD bias reservoir cap | per L22 datasheet | - | 0402 | SLCD charge-pump; value from datasheet LCD section |
| C4 | 1 | LCD bias reservoir cap | per L22 datasheet | - | 0402 | SLCD charge-pump |
| C5 | 1 | LCD bias reservoir cap | per L22 datasheet | - | 0402 | SLCD charge-pump |
| C6 | 1 | LCD bias reservoir cap | per L22 datasheet | - | 0402 | SLCD charge-pump |
| C7 | 1 | Decoupling cap | 100nF X7R | - | 0402 | VDD decoupling near MCU |
| C8 | 1 | Decoupling cap | 100nF X7R | - | 0402 | VDD decoupling near NFC front-end |
| C9 | 1 | Decoupling cap | 1uF X7R | - | 0603 | Bulk decoupling |
| C10 | 1 | Decoupling cap | 100nF X7R | - | 0402 | VDD decoupling near secure element |
| R1 | 1 | Pull-up resistor | 10k | - | 0402 | I2C SDA pull-up (secure element bus) |
| R2 | 1 | Pull-up resistor | 10k | - | 0402 | I2C SCL pull-up (secure element bus) |
| SW1 | 1 | Power/show button | Tactile (keyfob) / dome (card) | - | SMD | Wake MCU and display code |
| J1 | 1 | SWD programming pads | 2x5 1.27mm or test pads | - | - | Program and debug; can be bed-of-nails pads |

A rendered version of this table is in [`kicad/bom-table.png`](kicad/bom-table.png).
Regenerate it with `python3 kicad/draw_bom.py`.
