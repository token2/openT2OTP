# Porting guide

The firmware core is hardware-agnostic C. To bring it up on a real target you
implement a small HAL and one board-specific display map. Everything above that
line already works and is verified against published test vectors.

## 1. The HAL contract

Implement the functions declared in `firmware/include/token2_hal.h`:

| Function | Responsibility | Notes |
| --- | --- | --- |
| `hal_rtc_get()` / `hal_rtc_set()` | UTC seconds; set by the config command | back with the MCU RTC or an external low-power RTC |
| `hal_nvm_read()` / `hal_nvm_write()` | tamper-resistant store for seed + config | MCU protected flash or a discrete secure element |
| `hal_rng()` | fill the 8-byte auth challenge | use a hardware TRNG, never a clock-seeded PRNG |
| `hal_ct_memcmp()` | constant-time compare | use the SE/crypto-lib helper if available |

A working mock implementation lives in `firmware/test/mock_hal.c` — use it as a
template, but replace the RNG with a real TRNG on hardware.

## 2. The segment-LCD map

`firmware/src/slcd.c` holds a portable 7-segment font and digit logic. The only
board-specific piece is where each segment physically lives on your glass. You
provide three functions the driver calls:

```c
void hal_slcd_set_digit(int position, uint8_t seg_bits); /* write 7 bits (gfedcba) */
void hal_slcd_commit(void);                              /* latch frame buffer */
void hal_slcd_blank(void);                               /* all off (sleep) */
```

**Building `hal_slcd_set_digit()`:**

1. Open your LCD glass datasheet. It has a pinout table mapping every segment
   (digit N, segment a..g) to a **(COM, SEG)** pin pair.
2. For each of the 7 bits in `seg_bits`, set or clear the matching bit in your
   MCU's SLCD frame buffer at the right (COM, SEG) location.
3. On the SAM L22 this means writing the SLCD `LCDDATAx` registers; on another
   MCU with an LCD controller, the equivalent frame-buffer registers.

The mapping is pure transcription from the datasheet table — no cleverness. Once
`make totp` passes on the host and your map is correct, the right digits appear.

## 3. NFC transport

`firmware/src/nfc_transport.c` is the integration shim:

- Call `nfc_on_activate()` once at power-up / field activation → runs `token2_init()`.
- Call `nfc_on_apdu(cmd, len, resp, cap)` for every received command APDU → returns
  the response APDU.

Wire it to your NFC front-end's ISO-DEP (ISO 14443-4) receive path. Because the
host sends **no applet SELECT**, answer APDUs directly on the basic channel after
activation. ISO-DEP is block-based (T=1), so you don't need the `61 XX` / `6C XX`
continuation handling that only applies to T=0 contact readers.

## 4. Main loop sketch

```
on wake (RTC tick or button press):
    t    = hal_rtc_get()
    code = totp_now(seed, seed_len, t, step_seconds, 6)
    slcd_show_code(code, 6)
    stay awake briefly, then slcd_blank() and deep-sleep

on NFC field detect:
    nfc_on_activate()
    loop: nfc_on_apdu(...) per received APDU   (host provisioning session)
```

## 5. Verify as you go

- `make test` / `make kat` / `make totp` / `make seed` / `make slcd` (or
  `make all`) must pass on the host before you flash.
- `make slcd` renders the digits as ASCII art — a quick way to sanity-check your
  font/segment intent before wiring real glass.
- After flashing, provision over NFC with any conformant host (e.g. `token2_config.py`) and confirm the
  displayed code matches a soft TOTP app seeded with the same secret.
