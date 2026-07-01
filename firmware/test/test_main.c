/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* test_main.c - acts as token2_config.py would, driving the firmware through
 * a full session: info -> auth -> write config -> write seed. */
#include "token2.h"
#include "sm4.h"
#include "token2_hal.h"
#include <stdio.h>
#include <string.h>

extern uint32_t hal_rtc_get(void);

static int fails=0;
#define CHECK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } else printf("ok: %s\n",msg);}while(0)

static uint16_t sw_of(const uint8_t*r,size_t n){ return (uint16_t)((r[n-2]<<8)|r[n-1]); }

/* build the 4-byte MAC the same way the host does */
static void mac_for(uint8_t ins,uint8_t p1,uint8_t p2,const uint8_t*body,size_t blen,uint8_t out[4]){
    uint8_t buf[256];
    buf[0]=0x80;buf[1]=ins;buf[2]=p1;buf[3]=p2;buf[4]=(uint8_t)blen;
    memcpy(buf+5,body,blen);
    sm4_cbc_mac(TOKEN2_DEVICE_SM4_KEY,buf,blen+5,out);
}

int main(void){
    uint8_t resp[300]; size_t n;
    token2_init();

    /* 1. get info: 80 41 00 00 Lc=02 body=02 11 */
    { uint8_t c[]={0x80,0x41,0x00,0x00,0x02,0x02,0x11};
      n=token2_process_apdu(c,sizeof c,resp,sizeof resp);
      CHECK(sw_of(resp,n)==SW_OK,"info returns 9000");
      printf("   serial='%.*s' utc=%u\n",resp[3],resp+4,hal_rtc_get()); }

    /* 2. challenge: 80 4B 08 00 01 00 (Lc=1 body=00) */
    uint8_t chal[8];
    { uint8_t c[]={0x80,0x4B,0x08,0x00,0x01,0x00};
      n=token2_process_apdu(c,sizeof c,resp,sizeof resp);
      CHECK(sw_of(resp,n)==SW_OK && n==10,"challenge returns 8 bytes + 9000");
      memcpy(chal,resp,8); }

    /* 3. auth finish: SM4(challenge||8 zeros), 80 CE 00 00 10 <16> */
    { uint8_t blk[16],ct[16],c[5+16];
      memcpy(blk,chal,8); memset(blk+8,0,8);
      sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,blk,ct,1);
      c[0]=0x80;c[1]=0xCE;c[2]=0;c[3]=0;c[4]=0x10; memcpy(c+5,ct,16);
      n=token2_process_apdu(c,sizeof c,resp,sizeof resp);
      CHECK(sw_of(resp,n)==SW_OK,"auth finish returns 9000"); }

    /* 4. write config (algo SHA1, 30s step, disp=1, set clock) */
    { uint8_t body[19]={0x81,0x11, 0x1F,0x01,0x01, 0x0F,0x04,
                        0x65,0x4F,0xE6,0x00,  /* utc */
                        0x86,0x06, 0x0A,0x01,0x01, 0x0D,0x01,0x1E};
      uint8_t mac[4],c[5+23]; size_t L=19+4;
      mac_for(0xD4,0x00,0x00,body,19,mac);
      c[0]=0x84;c[1]=0xD4;c[2]=0;c[3]=0;c[4]=(uint8_t)L;
      memcpy(c+5,body,19); memcpy(c+5+19,mac,4);
      n=token2_process_apdu(c,5+L,resp,sizeof resp);
      CHECK(sw_of(resp,n)==SW_OK,"write config returns 9000");
      CHECK(hal_rtc_get()==0x654FE600,"clock was set from config TLV"); }

    /* re-auth (config write may reset session on real device; here seed was
       cleared due to restricted_sync, so authenticate again then write seed) */
    { uint8_t c[]={0x80,0x4B,0x08,0x00,0x01,0x00};
      n=token2_process_apdu(c,sizeof c,resp,sizeof resp); memcpy(chal,resp,8); }
    { uint8_t blk[16],ct[16],c[5+16];
      memcpy(blk,chal,8); memset(blk+8,0,8);
      sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,blk,ct,1);
      c[0]=0x80;c[1]=0xCE;c[2]=0;c[3]=0;c[4]=0x10; memcpy(c+5,ct,16);
      token2_process_apdu(c,sizeof c,resp,sizeof resp); }

    /* 5. write 20-byte seed: SM4-ECB with 9797-1 pad -> 32-byte ct, Lc=0x24 */
    { uint8_t seed[20]; uint8_t padded[32]; uint8_t ct[32]; uint8_t mac[4];
      uint8_t c[5+36]; size_t L; int i;
      for(i=0;i<20;i++) seed[i]=(uint8_t)(0x40+i);
      memcpy(padded,seed,20); padded[20]=0x80; memset(padded+21,0,11);
      sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,padded,ct,2);
      mac_for(0xC5,0x01,0x00,ct,32,mac);
      L=32+4;
      c[0]=0x84;c[1]=0xC5;c[2]=0x01;c[3]=0x00;c[4]=(uint8_t)L;
      memcpy(c+5,ct,32); memcpy(c+5+32,mac,4);
      n=token2_process_apdu(c,5+L,resp,sizeof resp);
      CHECK(sw_of(resp,n)==SW_OK,"write 20-byte seed (Lc=0x24) returns 9000"); }

    /* 6. negative: bad MAC must be rejected */
    { uint8_t body[19]={0x81,0x11,0x1F,0x01,0x01,0x0F,0x04,0,0,0,0,
                        0x86,0x06,0x0A,0x01,0x01,0x0D,0x01,0x1E};
      uint8_t badmac[4]={0,0,0,0}; uint8_t c[5+23]; size_t L=23;
      c[0]=0x84;c[1]=0xD4;c[2]=0;c[3]=0;c[4]=(uint8_t)L;
      memcpy(c+5,body,19); memcpy(c+5+19,badmac,4);
      n=token2_process_apdu(c,5+L,resp,sizeof resp);
      CHECK(sw_of(resp,n)==SW_AUTH_LOCK,"bad MAC rejected with 6983"); }

    printf("\n%s (%d failures)\n", fails?"TESTS FAILED":"ALL TESTS PASSED", fails);
    return fails?1:0;
}
