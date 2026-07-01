/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#ifndef TOKEN2_H
#define TOKEN2_H
#include <stdint.h>
#include <stddef.h>

/* Fixed device SM4 key from PROTOCOL.md. In real hardware this belongs in
 * SE key storage, not a code constant. Kept here to match the reference. */
extern const uint8_t TOKEN2_DEVICE_SM4_KEY[16];

/* Status words */
#define SW_OK        0x9000
#define SW_AUTH_LOCK 0x6983
#define SW_WRONG_LEN 0x6700
#define SW_INS_NG    0x6D00
#define SW_CLA_NG    0x6E00
#define SW_DATA_NG   0x6A80

/* Persistent device state layout (opaque to the transport). */
typedef struct {
    char     serial[16];      /* ASCII serial, e.g. "86596220001234" */
    uint8_t  serial_len;
    uint8_t  seed[63];
    uint8_t  seed_len;
    uint8_t  hmac_algo;       /* 1=SHA1 2=SHA256 */
    uint8_t  time_step;       /* wire byte: 0x1E=30s 0x3C=60s */
    uint8_t  display_timeout; /* wire byte 0..3 = 15/30/60/120s (host --sleep 1..4 maps to this) */
    uint8_t  key_locked;      /* if set, auth always fails -> 6983 */
    uint8_t  restricted_sync; /* if set, config write clears seed */
} token2_state_t;

/* Process one command APDU, produce response APDU (data||SW).
 * apdu:  raw command bytes (CLA INS P1 P2 [Lc data] [Le])
 * resp:  caller buffer, >= 260 bytes
 * returns response length (data + 2 SW bytes). */
size_t token2_process_apdu(const uint8_t *apdu, size_t apdu_len,
                           uint8_t *resp, size_t resp_cap);

/* Load state from NVM into the session at boot. */
void token2_init(void);

/* Read provisioned TOTP params for the display path. Returns 1 if seed present.
 * Does not expose the seed over the wire protocol. */
int token2_get_totp_params(uint8_t *seed, uint8_t *seed_len,
                           uint8_t *algo, uint8_t *step);

#endif
