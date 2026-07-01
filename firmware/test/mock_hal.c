/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#include "token2_hal.h"
#include <string.h>
#include <stdlib.h>

static uint32_t g_rtc = 1700000000u;
static uint8_t  g_nvm[4096];

uint32_t hal_rtc_get(void){ return g_rtc; }
void     hal_rtc_set(uint32_t s){ g_rtc=s; }
int  hal_nvm_read (uint16_t a,void*b,size_t n){ memcpy(b,g_nvm+a,n); return 0; }
int  hal_nvm_write(uint16_t a,const void*b,size_t n){ memcpy(g_nvm+a,b,n); return 0; }
void hal_rng(uint8_t*o,size_t n){ size_t i; for(i=0;i<n;i++) o[i]=(uint8_t)(0x11*(i+1)); } /* deterministic for test */
int  hal_ct_memcmp(const void*a,const void*b,size_t n){
    const uint8_t*x=a,*y=b; uint8_t d=0; size_t i;
    for(i=0;i<n;i++) d|=x[i]^y[i]; return d?1:0;
}
