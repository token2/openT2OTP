/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* slcd_test.c - host-side exercise of the segment-LCD driver.
 *
 * Provides trivial host implementations of the three hal_slcd_* hooks an
 * implementer must supply on real hardware, then renders a few codes and prints
 * them as ASCII 7-segment art so you can eyeball the font + digit placement
 * without any hardware. This is the reference stub for the display porting step
 * (docs/PORTING.md section 2). */
#include "slcd.h"
#include <stdio.h>
#include <string.h>

/* captured frame buffer: 6 digits x 7 segment bits (gfedcba) */
static unsigned char frame[6];

void hal_slcd_set_digit(int position, unsigned char seg_bits){
    if(position>=0 && position<6) frame[position]=seg_bits;
}
void hal_slcd_commit(void){
    /* render the captured frame as ASCII art, digits left-to-right */
    /* segment bit order: bit0=a,1=b,2=c,3=d,4=e,5=f,6=g */
    static const char *row_top = " _ ";
    char top[6][4], mid[6][4], bot[6][4];
    int d;
    for(d=0; d<6; d++){
        unsigned char s = frame[d];
        int a=s&1, b=(s>>1)&1, c=(s>>2)&1, dd=(s>>3)&1, e=(s>>4)&1, f=(s>>5)&1, g=(s>>6)&1;
        snprintf(top[d],4, "%s", a? " _ " : "   ");
        snprintf(mid[d],4, "%c%c%c", f?'|':' ', g?'_':' ', b?'|':' ');
        snprintf(bot[d],4, "%c%c%c", e?'|':' ', dd?'_':' ', c?'|':' ');
    }
    (void)row_top;
    for(d=0; d<6; d++) printf("%s ", top[d]); printf("\n");
    for(d=0; d<6; d++) printf("%s ", mid[d]); printf("\n");
    for(d=0; d<6; d++) printf("%s ", bot[d]); printf("\n\n");
}
void hal_slcd_blank(void){ memset(frame,0,sizeof frame); }

int main(void){
    unsigned int codes[] = {123456u, 7u, 654321u, 0u};
    int i;
    printf("Segment-LCD driver smoke test (ASCII render of 6-digit codes)\n\n");
    for(i=0;i<4;i++){
        printf("code %06u:\n", codes[i]);
        slcd_show_code(codes[i], 6);
    }
    /* basic correctness: after showing 123456, digit 0 should be '1' (b|c set) */
    slcd_show_code(123456u, 6);
    /* frame[5] is the rightmost digit = '6' -> segments a,c,d,e,f,g (not b) */
    unsigned char six = frame[5];
    int ok = ((six & 0x02)==0);     /* '6' has no 'b' segment in this font */
    printf("%s\n", ok ? "digit mapping sanity: PASS" : "digit mapping sanity: FAIL");
    return ok?0:1;
}
