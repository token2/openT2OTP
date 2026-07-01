# Wire protocol reference

The device firmware in this repo answers the **Token2 single-profile programmable
TOTP token** wire protocol. The canonical, authoritative specification is the
`PROTOCOL.md` published with the host configurator:

> https://github.com/token2/token2_config.py/blob/main/PROTOCOL.md

This page is a short orientation so you can read the firmware; always defer to the
canonical spec for exact byte layouts.

## Transport

- NFC Type-4 / ISO 7816 APDUs over a contactless (or contact) PC/SC reader.
- **No applet SELECT** — the token answers APDUs directly after activation.
- ISO-DEP (block-based) contactless; T=0 continuation only matters on contact
  readers.

## Cryptographic primitives

- **SM4** (GB/T 32907) — encrypts the seed and the auth response, and provides the
  per-command MAC. Implemented in `firmware/src/sm4.c`, verified against the
  GM/T 0002 known-answer test.
- The protocol uses a **single fixed device key** (documented in the canonical
  spec). See [`SECURITY.md`](SECURITY.md) for why that matters.

## Command families

**Plain (CLA `0x80`, no auth):**
- get info (serial + on-device UTC time)
- get challenge (8 random bytes)
- auth finish (verify `SM4(challenge ‖ 8 zero bytes)`)

**Secure (CLA `0x84`, requires auth + a 4-byte SM4-CBC-MAC):**
- write seed (SM4-ECB body, ISO/IEC 9797-1 padding)
- write config (plaintext TLV: display timeout, UTC clock, HMAC algo, time step)

The firmware's dispatcher (`firmware/src/token2.c`) implements all of these and is
exercised end-to-end by `firmware/test/test_main.c` (`make test`).

## TOTP generation

Once a seed and config are written, the device generates codes per **RFC 6238**
(`firmware/src/totp.c`, verified against the RFC's published test vectors) and
shows them on the segment LCD.
