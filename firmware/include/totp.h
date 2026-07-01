/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#ifndef TOTP_H
#define TOTP_H
#include <stdint.h>
#include <stddef.h>
/* algo: 1 = HMAC-SHA1, 2 = HMAC-SHA256 */
uint32_t totp_now_algo(const uint8_t *seed, uint8_t seed_len,
                       uint32_t unix_time, uint32_t step, int digits, int algo);
/* Back-compat: SHA1. */
uint32_t totp_now(const uint8_t *seed, uint8_t seed_len,
                  uint32_t unix_time, uint32_t step, int digits);
#endif
