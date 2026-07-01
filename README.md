<p align="center">
  <img src="docs/logo.svg" alt="openT2OTP" width="420">
</p>

# openT2OTP — open-source programmable TOTP hardware token

An open-source **firmware + hardware reference** for a self-contained, NFC-programmable
TOTP hardware token: a small device with a segment LCD that shows a rotating
6-digit code, keeps time for 5–8 years on a coin cell (depending on model), and is provisioned over NFC using the
documented Token2 programmable-token wire protocol. Any host that implements the
protocol can provision it; the open [`token2_config.py`](https://github.com/token2/token2_config.py)
tool is one such host.

This is the **actual firmware core our original programmable TOTP tokens run
on** — not a reconstruction. We've decided to open-source it because there is
nothing to hide in how a TOTP token works: the security of TOTP rests on the
secret seed, not on obscurity of the device, so opening the design costs nothing
and lets anyone audit, learn from, or build on it.

### A little history

This code has a long, stable lineage. The core was **finalized in 2020** and has
run our programmable tokens ever since. It has changed very little since — the
**most recent change was in 2025**, when we removed the lockout that triggered on
a wrong customer key (the handshake now simply requires re-challenging rather than
locking; see [`docs/SECURITY.md`](docs/SECURITY.md)). What's published here is
**exactly the core our current hardware runs** — same protocol, same crypto, same
behavior.

**About the name.** "openT2OTP" is the name we chose *for the open-source release*.
Internally, this firmware was developed and shipped under a different codename; we
picked a plain, descriptive public name rather than carry the internal one over. So
if the code, comments, or history feel like they predate the "openT2OTP" label —
they do. The name is new; the core is not.

---

## ⚠️ Read this first: TOTP is not phishing-resistant

**TOTP (the 6-digit code) does not protect you from phishing.** A convincing fake
login page can ask for your code and replay it to the real site within the 30-second
window. The code has no idea *which* website it's being typed into, so it can't tell
a real site from a fake one.

If you need real phishing resistance, **use a FIDO2 / WebAuthn security key instead.**
FIDO2 cryptographically binds each login to the real site's origin, so a phished
credential is useless to an attacker. Token2 also open-sources FIDO2 work — prefer
that for any account that supports security keys.

Use this TOTP token when a service only supports TOTP, or as a convenient second
factor where you accept the phishing trade-off. It is **not** a substitute for a
FIDO2 key.

See [`docs/SECURITY.md`](docs/SECURITY.md) for the full threat model.

---

## What's in here

```
firmware/     portable C firmware (protocol + SM4 + TOTP + segment-LCD driver)
hardware/     LCD wiring, BOM, PCB floorplan, antenna & casing notes
docs/         protocol reference, security model, porting guide
```

- **Firmware** — implements the device side of the Token2 single-profile TOTP
  wire protocol (the ISO 7816 APDUs, SM4 handshake, MAC, seed and config writes),
  plus RFC 6238 TOTP generation and a segment-LCD display driver. It interoperates
  with any conformant host; hardware-agnostic C, ported by filling in a small HAL.
- **Hardware** — a reference design built around an MCU with an integrated
  segment-LCD controller and RTC, an NFC front-end, and a coin cell. Keyfob-dongle
  form factor as the main target, card as a documented variation.

The casing and board outline here are **deliberately our own** — this is a
functional reference, not a clone of anyone's industrial design.

## Verified, not just written

Every cryptographic and framing claim in the firmware is checked against a
published test vector or an exact round-trip:

```
cd firmware
make test     # full Token2 wire-protocol session       -> ALL TESTS PASSED
make kat      # SM4 vs GM/T 0002 known-answer test        -> PASS
make totp     # TOTP vs RFC 6238 test vectors             -> PASS
make seed     # seed framing round-trips (all 1..63 lens) -> PASS
make slcd     # segment-LCD driver renders digits         -> PASS
make all      # everything
```

You need only a C compiler (`gcc`/`clang`) and `make`.

## Quick start

1. **Read the firmware** — start at [`firmware/README`](firmware/README.md) for the
   protocol implementation, then [`docs/PROTOCOL.md`](docs/PROTOCOL.md) for the wire
   format.
2. **Read the hardware reference** — [`hardware/HARDWARE.md`](hardware/HARDWARE.md)
   covers the LCD, wiring, BOM, and PCB design.
3. **Port to your target** — implement the HAL in
   [`docs/PORTING.md`](docs/PORTING.md) (RTC, NVM, RNG, LCD segment map).
4. **Provision a token** — use [`token2_config.py`](https://github.com/token2/token2_config.py)
   over NFC/PC-SC to write a seed and configuration.

## On sourcing components

The BOM lists **mainstream, easy-to-source example parts** (a Microchip SAM L22
MCU, a standard NFC front-end, a transflective segment LCD, a CR2032). You can
build the reference with off-the-shelf parts from any distributor.

**We ordered components manufactured specifically for this code to run on** — the
exact parts our production tokens use. They're proven, but **not always easily
sourceable** (some are factory-tailored equivalents of a mainstream component).
Our **research suggests standard off-the-shelf components should work as well**, so
the BOM lists mainstream, buy-anywhere equivalents as examples, not requirements:
anything with an integrated segment-LCD controller, an RTC, and enough flash should
do. If small changes are needed to run on a particular off-the-shelf part, **we'll
update the design — and community updates, or even just reports of such
incompatibilities, are very welcome.**

## License

- **Firmware:** MIT — see [`LICENSE`](LICENSE).
- **Hardware documentation & design:** CERN-OHL-P v2 — see
  [`hardware/LICENSE-HARDWARE`](hardware/LICENSE-HARDWARE).

## Contributing

Issues and PRs welcome — see [`CONTRIBUTING.md`](CONTRIBUTING.md). Good first areas:
additional glass pin-maps, a routed KiCad PCB for the reference board, and reports
of any off-the-shelf components that need small tweaks to run the core (see the
sourcing note).

## Provenance

This is the **real device-side firmware from our programmable TOTP tokens**,
implementing the **Token2 single-profile programmable TOTP wire protocol**. The
protocol and a reference host are published openly at
[`token2_config.py`](https://github.com/token2/token2_config.py) (see its
`PROTOCOL.md`); that tool is one conformant host. The core here was finalized in
2020, last changed in 2025 (wrong-key lockout removed), and is what our current
hardware runs.
