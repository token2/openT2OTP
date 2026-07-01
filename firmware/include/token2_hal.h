/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#ifndef TOKEN2_HAL_H
#define TOKEN2_HAL_H
#include <stdint.h>
#include <stddef.h>

/* ---- What every hardware target must implement ---------------------------
 * The token core is transport- and MCU-agnostic. Port these to your SE/MCU. */

/* RTC: seconds since UNIX epoch (UTC). Must survive field battery life and
 * be settable by the config command. On an SE without an RTC, drive this from
 * an external low-power RTC (e.g. RV-3028-C7) over I2C. */
uint32_t hal_rtc_get(void);
void     hal_rtc_set(uint32_t unix_seconds);

/* Secure, tamper-resistant non-volatile store for the seed + config + auth
 * lock flag. On a secure element this is the SE's protected flash/EEPROM. */
int  hal_nvm_read (uint16_t addr, void *buf, size_t len);
int  hal_nvm_write(uint16_t addr, const void *buf, size_t len);

/* Cryptographically secure RNG for the 8-byte auth challenge.
 * On an SE, use the certified TRNG; never a PRNG seeded from the clock. */
void hal_rng(uint8_t *out, size_t len);

/* Optional: constant-time compare provided by SE crypto lib if available. */
int  hal_ct_memcmp(const void *a, const void *b, size_t len);

#endif
