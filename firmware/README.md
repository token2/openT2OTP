# Firmware — device-side TOTP token

> Part of the [openT2OTP](../README.md) repository. See the root README for the
> project overview and the important note that **TOTP is not phishing-resistant**
> (use FIDO2 for phishing resistance).

This is the **actual device-side firmware from our programmable TOTP tokens** —
a portable, hardware-agnostic implementation of the Token2 single-profile
programmable TOTP token **wire protocol**. Any host that speaks the protocol can
provision the device; the open `token2_config.py` is one such host, and its
`PROTOCOL.md` is the canonical spec this firmware implements. A host writes a seed
and configuration to the token over NFC/PC-SC; this firmware is what answers those
APDUs. The core was finalized in 2020 and is verified by host-side tests that
replay a full session and check the crypto against published test vectors.

> **Provenance & scope.** This is our real production core, now open-sourced —
> not a reconstruction. The "device SM4 key" in `PROTOCOL.md` is a fixed, publicly
> documented value (intentionally so — see `../docs/SECURITY.md`); anyone forking
> this for a *different* product who wants real device authentication should move
> to a per-device secret key. As-is, this is exactly what our tokens run.

## What it implements

Plain commands (CLA `0x80`, no auth):
- `0x41` get info — returns serial + on-device UTC time
- `0x4B/P1=08` get challenge — 8 random bytes
- `0xCE` auth finish — verifies `SM4(challenge ‖ 8 zero bytes)`

Secure commands (CLA `0x84`, 4-byte SM4-CBC-MAC required, auth required):
- `0xC5/P1=01` write seed — SM4-ECB body, 9797-1 padding, both the general and
  the 32-byte "extra pad block" framings
- `0xD4` write config — 19-byte plaintext TLV: display timeout, UTC clock set,
  HMAC algo (SHA1/SHA256), time step (30s/60s)

Behaviours from the spec: MAC header uses plain class `0x80` even for `0x84`
APDUs; `restricted_sync` clears the seed on config write; bad auth / locked key
returns `6983`; wrong length returns `6700`.

## Layout

```
include/  sm4.h  token2.h  token2_hal.h     public API + HAL contract
src/      sm4.c                             SM4 + protocol CBC-MAC (KAT-verified)
          token2.c                          APDU dispatcher (the firmware core)
          nfc_transport.c                   deploy shim for SE applet / MCU frontend
test/     mock_hal.c test_main.c kat.c      host tests
Makefile
```

## Build & test (host)

```
make test     # replays a full host session against the core -> ALL TESTS PASSED
make kat      # SM4 vs GM/T 0002 known-answer test -> PASS
```

The core (`src/sm4.c`, `src/token2.c`) has no host dependencies beyond the four
HAL functions in `token2_hal.h`. Cross-compile those two files with your target
toolchain and supply a real HAL.

## Porting: the HAL you must provide

| Function | Responsibility | On a secure element |
| --- | --- | --- |
| `hal_rtc_get/set` | UTC seconds, set by config cmd | external low-power RTC (SE usually has none) |
| `hal_nvm_read/write` | tamper-resistant store for seed+config | SE protected EEPROM/flash |
| `hal_rng` | 8-byte challenge | certified on-die TRNG (never a clock-seeded PRNG) |
| `hal_ct_memcmp` | constant-time compare | SE crypto-lib helper |

## Hardware recommendations

The protocol has three hard requirements that drive the part choice:
**(1) contactless ISO 14443-4 (NFC Type-4) card emulation answering raw APDUs
with no applet SELECT, (2) SM4** (the GB/T 32907 cipher — not AES), and
**(3) a timekeeping source** for TOTP.

### Important constraint: SM4 availability

Mainstream JavaCard/JCOP secure elements (e.g. NXP JCOP 4.5 on P71) expose
AES / 3DES / RSA / ECC / SHA through the JavaCard crypto API but **do not expose
SM4** as a standard primitive; JCOP lists AES, 3DES, RSA, ECC and Korean SEED,
not SM4. So on a certified SE you have two realistic routes:

- **A native SM4 execution path.** NXP JCOP 4.5 P71 ships *SecureBox*, a native
  C sandbox inside the secure enclave for exactly the algorithms the JavaCard
  API lacks. Compile `sm4.c` into a SecureBox module and call it from the
  applet. This keeps the key and cipher inside the certified boundary. Requires
  an NXP SecureBox NDA/toolchain (partners like Cryptnox do this commercially).
- **Software SM4 in the applet.** Run `sm4.c` as plain applet bytecode. Simplest
  to build; the seed still lives in SE-protected EEPROM, but the SM4 operation
  itself isn't hardware-accelerated or side-channel-hardened. Acceptable for a
  reference/interop token, not for a high-assurance product.

### Option 1 — Certified secure element (closest to the real product)

- **NXP JCOP 4.5 on P71 (SmartMX3)** — ARM SC300 + FAME3 crypto coprocessor,
  Java Card 3.0.5/3.1, GlobalPlatform 2.3.x, dual-interface (contact +
  contactless up to 848 kbit/s), EAL6+ class hardware, PUF-backed identity.
  Register your applet with the **default-selected** privilege so the card
  routes APDUs to it without a SELECT AID — that is what lets a host like `token2_config.py`
  talk to it directly, matching the "no applet SELECT" posture in the protocol.
  SM4 via SecureBox as above.
- **Infineon SLE78 / SECORA ID** or **Samsung S3K** are equivalent-class
  alternatives if you prefer their toolchains; same SM4 caveat applies.

Pros: real tamper resistance, certified TRNG, protected key store, exactly the
device category Token2 ships. Cons: NDA-gated SDKs, needs an external RTC.

### Option 2 — Secure MCU + NFC frontend (fastest to prototype, fully open)

- **MCU:** an Arm Cortex-M with on-chip AES/crypto and secure storage, e.g.
  **STM32L5 / U5** (TrustZone-M, on-chip RTC, low power) or **NXP LPC55S69**
  (TrustZone, PUF, casper crypto). The on-chip RTC satisfies requirement (3)
  directly; run `sm4.c` in software (cheap on M33).
- **NFC frontend in card-emulation:** **ST25DV** (dynamic NFC tag with I²C, if
  you drive the tag memory) or a full CE-capable controller like **NXP PN7160 /
  PN5180** or **ST ST25R3916** presenting an **ISO-DEP (14443-4)** endpoint.
  ISO-DEP is block-based (T=1); feed each deblocked APDU straight to
  `nfc_on_apdu()` — no `61 XX`/`6C XX` handling needed (that's only for T=0
  contact readers).

Pros: no NDA, open toolchain, on-chip RTC, easy to debug over the same PC/SC
path a conformant host uses. Cons: not certified tamper-resistant; a discrete secure
element (e.g. **ATECC608** / **NXP SE050** / **Microchip TA100**) can be added
over I²C to hold the seed if you want a protected key store — note those provide
AES/ECC, so SM4 still runs on the MCU.

### Option 3 — Add a dedicated secure element to Option 2

Pair the MCU with **NXP SE050** or **Microchip ATECC608B** purely as a protected
key/seed vault and TRNG source (map `hal_nvm_write` for the seed and `hal_rng`
to it), while SM4 and the APDU logic stay on the MCU. Good middle ground:
open toolchain, hardware-protected secrets.

## Deployment shim

`src/nfc_transport.c` shows the two integration points:
`nfc_on_activate()` (call at power-up → `token2_init()`) and
`nfc_on_apdu(cmd, len, resp, cap)` (call per received APDU). For an SE applet,
call `token2_process_apdu()` from `process(APDU)`; for an MCU, call it from your
ISO-DEP receive callback.

## Security notes (read before shipping anything)

- The device SM4 key is **fixed and public** in the protocol, so device
  authentication only proves the host knows a published constant — it is not a
  real secret. Any product reusing it has no meaningful anti-cloning or
  anti-tamper authentication at the protocol level. A real deployment should
  move to a per-device key and a challenge/response the attacker can't precompute.
- Keep the seed inside the secure boundary (SE EEPROM or a discrete SE); never
  expose a read-seed command (the protocol has none — preserve that).
- Use a certified TRNG for the challenge; a predictable challenge lets a
  recorded `SM4(challenge‖0)` be replayed.
```
