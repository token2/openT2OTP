/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* Round-trip: write seed, then reach into state to confirm exact bytes.
   We include token2.c's state by declaring an accessor via extern is not
   possible (static S). Instead we re-run the decode path standalone to prove
   the padding-strip recovers the exact seed for each length/form. */
#include "sm4.h"
#include <stdio.h>
#include <string.h>
extern const uint8_t TOKEN2_DEVICE_SM4_KEY[16];

static void dec_block(const uint8_t k[16],const uint8_t in[16],uint8_t out[16]){
  uint32_t rk[32],rr[32]; int i; sm4_key_schedule(k,rk);
  for(i=0;i<32;i++) rr[i]=rk[31-i]; sm4_ecb_block(rr,in,out);
}
/* mirror device strip logic */
static int strip(const uint8_t*pt,int n){
  int slen=-1,i;
  for(i=n;i>0;i--){ if(pt[i-1]==0x80){slen=i-1;break;} if(pt[i-1]!=0x00)break; }
  return slen;
}
int main(void){
  const uint8_t KEY[16]={0x8A,0xD2,0x06,0x88,0x3C,0xA3,0x69,0x48,0x2A,0xB2,0x71,0x82,0xB6,0xE8,0x32,0x24};
  int lens[]={1,10,15,16,17,20,31,32,33,47,48,63};
  int fails=0;
  for(int li=0; li<12; li++){
    int L=lens[li];
    uint8_t seed[63]; for(int i=0;i<L;i++) seed[i]=(uint8_t)(0x30+i);
    /* host framing */
    uint8_t padded[80]; int plen;
    if(L==32){ memcpy(padded,seed,32); padded[32]=0x80; memset(padded+33,0,15); plen=48; }
    else { memcpy(padded,seed,L); padded[L]=0x80; int p=L+1; while(p%16)padded[p++]=0; plen=p; }
    uint8_t ct[80]; sm4_ecb_encrypt(KEY,padded,ct,plen/16);
    /* device decode */
    uint8_t pt[80]; for(int i=0;i<plen;i+=16) dec_block(KEY,ct+i,pt+i);
    int rec=strip(pt,plen);
    int ok = (rec==L) && (memcmp(pt,seed,L)==0);
    printf("L=%2d plen=%2d recovered=%2d %s\n", L, plen, rec, ok?"OK":"MISMATCH");
    if(!ok) fails++;
  }
  printf("%s\n", fails?"ROUND-TRIP FAILURES":"all seeds round-trip exactly");
  return fails;
}
