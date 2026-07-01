/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#ifndef SM4_H
#define SM4_H
#include <stdint.h>
#include <stddef.h>

void sm4_key_schedule(const uint8_t key[16], uint32_t rk[32]);
void sm4_ecb_block(const uint32_t rk[32], const uint8_t in[16], uint8_t out[16]);
void sm4_ecb_encrypt(const uint8_t key[16], const uint8_t *in, uint8_t *out, size_t nblocks);
void sm4_cbc_mac(const uint8_t key[16], const uint8_t *msg, size_t len, uint8_t mac[4]);

#endif
