/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* slcd.c - segment LCD driver for a 6-digit 7-segment TOTP display.
 *
 * Targets the SAM L22 on-chip SLCD controller (register-level writes go in the
 * HAL below). The panel is a 6-digit static/multiplexed 7-seg glass; each digit
 * is 7 segments (a..g) plus optional decimal point. We use a 4-COM x N-SEG
 * layout typical of a 1/4-duty, 1/3-bias glass:
 *
 *      a
 *    -----
 *  f |   | b
 *    | g |
 *    -----
 *  e |   | c
 *    |   |
 *    -----
 *      d
 *
 * On the SAM L22 you assign each segment to a (COM, SEG) pin pair in the glass
 * datasheet's pinout table, then set/clear the matching bit in the SLCD frame
 * buffer. Below we keep a portable segment bitmap and delegate the actual
 * COM/SEG bit placement to hal_slcd_set_digit(), which you fill in from your
 * glass's pin map. */
#include "slcd.h"
#include "token2_hal.h"

/* 7-seg font, bit order: gfedcba (bit0=a .. bit6=g) */
static const uint8_t FONT[10] = {
    0x3F, /*0*/ 0x06, /*1*/ 0x5B, /*2*/ 0x4F, /*3*/ 0x66, /*4*/
    0x6D, /*5*/ 0x7D, /*6*/ 0x07, /*7*/ 0x7F, /*8*/ 0x6F  /*9*/
};

/* Provided by the SAM L22 HAL: write the 7 segment bits for one digit position
 * into the SLCD frame buffer using the glass's COM/SEG mapping. */
extern void hal_slcd_set_digit(int position, uint8_t seg_bits);
extern void hal_slcd_commit(void);      /* latch frame buffer to the panel */
extern void hal_slcd_blank(void);       /* all segments off (sleep) */

void slcd_show_code(uint32_t code, int digits){
    int i; uint32_t v=code;
    /* right-aligned, leading zeros (standard for OTP tokens) */
    for(i=digits-1;i>=0;i--){
        hal_slcd_set_digit(i, FONT[v%10]);
        v/=10;
    }
    /* blank any unused positions to the left (max 6) */
    for(i=digits;i<6;i++) hal_slcd_set_digit(i,0x00);
    hal_slcd_commit();
}

void slcd_blank(void){ hal_slcd_blank(); }
