# Security model

This document is deliberately blunt about what this token does and does not
protect against. Open-sourcing the design does not weaken it: TOTP security
depends on the secrecy of the seed, not on hiding how the device works.

## What TOTP gives you

TOTP is a **second factor**: a rotating 6-digit code derived from a shared secret
(the seed) and the current time (RFC 6238). Combined with a password, it raises
the bar over passwords alone — an attacker who only has your password can't log in
without also having a current code.

## What TOTP does NOT give you

**Phishing resistance.** This is the important one.

The 6-digit code is just a number. It carries no information about *where* it's
being entered. If an attacker builds a convincing fake login page:

1. You type your password and current code into the fake page.
2. The attacker's server immediately relays both to the real site.
3. Within the 30-second window, the code is still valid — the attacker is in.

No amount of care on the device side fixes this, because the weakness is in the
protocol: the code can't distinguish the real site from a look-alike. Push-based
and SMS second factors share the same flaw.

## Use FIDO2 / WebAuthn for phishing resistance

FIDO2 solves exactly this. During registration and every login, the authenticator
performs a challenge-response that is **cryptographically bound to the website's
origin** (its real domain). A FIDO2 credential created for `example.com` simply
will not produce a valid assertion for `examp1e.com` — the browser and
authenticator refuse. A phished FIDO2 login yields the attacker nothing.

| Property | TOTP (this token) | FIDO2 / WebAuthn |
| --- | --- | --- |
| Second factor | yes | yes |
| Phishing-resistant | **no** | **yes** |
| Bound to site origin | no | yes |
| Replayable within a window | yes (~30 s) | no |
| Works offline / no network on device | yes | yes |
| Shared secret on server | yes (seed) | no (public key only) |

**Recommendation:** for any account that supports security keys, use a FIDO2 key.
Token2 open-sources FIDO2 work too — prefer it. Use this TOTP token only where a
service is TOTP-only, or as a convenience factor where you knowingly accept the
phishing trade-off.

## Seed protection: write-only, and where it physically lives

The seed is the entire secret. Two separate properties matter, and they are not the
same thing:

**1. Write-only over the wire (guaranteed by this firmware).** The protocol exposes
exactly five commands — get-info, get-challenge, auth-finish, write-seed, and
write-config. **There is no read-seed command, and no response ever contains seed
bytes.** You can write a seed; you can never read one back out over NFC/APDUs.
`cmd_info` returns only the serial number and the clock. This is a hard property of
the command dispatcher in `token2.c` — **if you fork this, do not add a seed-read
command, and do not extend get-info to echo the seed.** A quick way to keep yourself
honest: grep the response paths for the seed buffer; nothing should ever copy
`S.seed` into a `resp` buffer.

**2. Extraction resistance in silicon (depends on YOUR hardware, not this code).**
Being write-only over the wire does **not** by itself mean the seed can't be
extracted. The firmware still has to store the seed somewhere and read it back
internally to compute the code (via `token2_get_totp_params()` → `totp_now_algo()`).
*Where* it is stored decides whether someone with physical access can recover it.
The firmware persists the seed through one HAL call, `hal_nvm_write()`, so the
security of the seed is entirely a property of **how you implement that HAL** on
your part. Concretely:

| Storage choice | Can an attacker with the device extract the seed? | Notes |
| --- | --- | --- |
| **Raspberry Pi Pico / plain MCU internal flash** | **Yes — assume it's extractable.** | RP2040/RP2350 flash is not tamper-resistant; SWD/debug can often be re-enabled and the flash dumped. Fine for a demo, **not** for a real token. |
| **MCU internal flash + debug locked (e.g. SAM L22 with security bit set)** | **Partially.** Raises the bar (readout protection, disabled debug), but on-die flash is still recoverable by a determined attacker with lab equipment. Acceptable for low-value use, not high-assurance. |
| **Discrete secure element (NXP SE050, Microchip ATECC608)** | **No, if used correctly.** The seed is written into the SE's protected key store and **never leaves it**. The HMAC/TOTP is computed *inside* the SE, so the seed is never present in MCU RAM. This is the design intent for a real token. |
| **SE with the key marked non-extractable + compute-in-place** | **No.** Strongest option: the seed is injected once, flagged non-exportable, and only ever used by the SE's own HMAC engine. Even compromised firmware cannot read it. |

**What this means for a real product.** To make "the seed cannot be extracted" a
true statement, implement `hal_nvm_write()`/the seed path against a **secure element**
and, ideally, do the HMAC **inside** the SE so the seed never enters MCU memory. On a
bare MCU (or a Pico), the seed is only as safe as that chip's flash protection —
which is to say, treat it as recoverable. This firmware gives you the write-only wire
guarantee for free; the silicon guarantee is your hardware choice.

**One caveat in the reference code you must address for an SE build.** The display
path reads the seed through `token2_get_totp_params()`, which in this reference copies
the raw seed into MCU RAM so `totp_now_algo()` can compute the code. That is correct
for a plain-MCU build but **defeats a secure element** — a seed sitting in RAM can be
recovered from a RAM dump or a firmware compromise, even if it's stored in an SE at
rest. For a real SE-backed token: keep the seed **inside** the SE, mark it
non-exportable, and replace the accessor + `totp_now_algo()` with a call to the SE's
own HMAC engine, so the plaintext seed is never present in MCU memory at all. The
accessor carries this note inline in `token2.c`.

**Zeroization.** The firmware wipes the stored seed buffer before writing a new seed
(no stale tail bytes from a longer previous seed), scrubs the decrypted-seed scratch
buffer off the stack after use, and clears the seed when a restricted-sync config
reprovision occurs. On an SE build these MCU-side wipes matter less (the seed
shouldn't be in MCU memory at all), but they are the right hygiene for the plain-MCU
reference.

> **Our production tokens** store the seed in a secure element, mark it
> non-exportable, and compute the HMAC inside the SE — so the seed is never in MCU
> RAM and cannot be extracted. The mainstream-equivalent BOM lists an SE for this
> reason; the `hardware/` reference documents the SE option (see the secure-element
> notes there).

## Device-level security notes

Even within TOTP's limits, a few things matter on the device:

- **Keep the seed inside the secure boundary.** The seed is the entire secret.
  Store it in tamper-resistant NVM (a discrete secure element, or the MCU's
  protected flash). The wire protocol has **no read-seed command** — preserve that;
  never add one.
- **The protocol's device key is fixed and public — by design.** The Token2 wire
  protocol authenticates the host with a documented constant key, and this
  reference matches that: the key is the same on every device and openly
  published, so device authentication proves only that the host knows a public
  value. This is intentional and not a secret we are protecting — see below.
- **Use a real TRNG for the challenge.** A predictable challenge lets a recorded
  handshake be replayed. Use the MCU/SE hardware RNG, never a clock-seeded PRNG.
- **Time integrity.** TOTP validity depends on the RTC. Large clock drift breaks
  code acceptance; a manipulated clock doesn't leak the seed but can cause denial
  of service. Use a low-ppm crystal and let the host re-sync time when provisioning.

## The device key: where to change it if you want it secret

For Token2 the device key is deliberately public, so where it physically lives is
irrelevant — flash, a `const` in the binary, or a key slot are equivalent when the
value is already documented. In this reference it is a plain constant:

```c
/* firmware/src/token2.c */
const uint8_t TOKEN2_DEVICE_SM4_KEY[16] = { 0x8A,0xD2, ... };
```

If you are **forking this for a different product** and want real anti-cloning
device authentication, this is the place to change it:

1. **Make the key per-device and secret.** Replace the compile-time constant with
   a value provisioned at manufacture into tamper-resistant storage, and read it
   through the HAL instead of referencing the constant:
   - add something like `hal_get_device_key(uint8_t out[16])` to `token2_hal.h`,
     backed by secure-element key storage, protected flash, or a PUF-derived key;
   - in `cmd_auth_finish()`, `mac_ok()`, and `cmd_seed()`, source the key from the
     HAL rather than from `TOKEN2_DEVICE_SM4_KEY`.
2. **Keep the challenge unpredictable** (already required — real TRNG).
3. **Update the host side too.** The host configurator must know each device's key
   (e.g. derived from the serial via a manufacturer secret), since it can no longer
   use one published constant.

None of this is needed to be compatible with the existing Token2 tooling — it is
only for forks that want the device key to be an actual secret.

## Retry / lockout behaviour

**This firmware has no wrong-key lockout, and that is acceptable for the public-key
protocol.** On a failed handshake, `cmd_auth_finish()` invalidates the current
challenge and returns `6983`; the host must request a fresh challenge to try again.
There is **no attempt counter, no backoff, and no permanent lock**. A `key_locked`
flag exists in device state and is honoured if set (returns `6983`), but nothing in
this firmware ever sets it — it is a hook, not an active mechanism.

> **History.** Earlier revisions of this core *did* lock on a wrong customer key.
> That lockout was **removed in 2025** — the most recent change to a core otherwise
> finalized in 2020. On the public-key protocol the lockout protected nothing (see
> below), and it could strand a token on an honest mistake, so it was taken out. The
> `key_locked` hook was left in place for forks that adopt a secret key.

Why this is fine here: the device key is public, so "guessing" the auth response is
meaningless — anyone can compute the correct answer directly. A lockout exists to
slow down *guessing a secret*, and there is no secret to guess. Its absence is not a
weakness in the Token2 model.

**If you move to a per-device secret key (above), add throttling.** In that case
unlimited fast retries would let an attacker attack the key, so you should:

- increment a persisted failure counter on each bad handshake;
- after N failures, set `S.key_locked` (and `persist()`), or impose an increasing
  delay before honouring the next challenge;
- optionally clear the counter on a successful auth.

The plumbing is already present — the check `if(S.key_locked) return SW_AUTH_LOCK;`
runs at the top of `cmd_auth_finish()`. You only need to add the code that *sets*
the flag / counter on repeated failure.

## Reporting issues

Please report security issues privately first — see [`../SECURITY.md`](../SECURITY.md)
in the repo root for the disclosure process.
