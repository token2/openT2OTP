/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#ifndef SLCD_H
#define SLCD_H
#include <stdint.h>
void slcd_show_code(uint32_t code, int digits);
void slcd_blank(void);
#endif
