/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* Drive the full provisioning sequence (challenge -> auth -> config -> seed)
   against the core over the wire protocol, proving the exact APDU sequence a
   real host sends works and yields a live TOTP. Transport-agnostic: the same
   sequence runs over NFC on the reference board. */
#include "token2.h"
#include "sm4.h"
#include "totp.h"
#include "token2_hal.h"
#include <stdio.h>
#include <string.h>

extern const uint8_t TOKEN2_DEVICE_SM4_KEY[16];
extern int token2_get_totp_params(uint8_t*,uint8_t*,uint8_t*,uint8_t*);

static uint16_t sw(const uint8_t*r,size_t n){return (r[n-2]<<8)|r[n-1];}
static void mac_for(uint8_t ins,uint8_t p1,uint8_t p2,const uint8_t*b,size_t bl,uint8_t o[4]){
  uint8_t buf[256]; buf[0]=0x80;buf[1]=ins;buf[2]=p1;buf[3]=p2;buf[4]=(uint8_t)bl;
  memcpy(buf+5,b,bl); sm4_cbc_mac(TOKEN2_DEVICE_SM4_KEY,buf,bl+5,o);
}

int main(void){
  uint8_t resp[300]; size_t n; int fails=0;
  token2_init();

  /* 1. challenge */
  { uint8_t c[]={0x80,0x4B,0x08,0x00,0x01,0x00};
    n=token2_process_apdu(c,sizeof c,resp,sizeof resp);
    if(sw(resp,n)!=0x9000){printf("challenge FAIL\n");fails++;} }
  uint8_t chal[8]; memcpy(chal,resp,8);

  /* 2. auth */
  { uint8_t blk[16],ct[16],c[5+16];
    memcpy(blk,chal,8);memset(blk+8,0,8);
    sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,blk,ct,1);
    c[0]=0x80;c[1]=0xCE;c[2]=0;c[3]=0;c[4]=0x10;memcpy(c+5,ct,16);
    n=token2_process_apdu(c,sizeof c,resp,sizeof resp);
    if(sw(resp,n)!=0x9000){printf("auth FAIL\n");fails++;} }

  /* 3. config: SHA1, 30s, set clock to 1700000000 */
  { uint32_t utc=1700000000u;
    uint8_t body[19]={0x81,0x11,0x1F,0x01,0x01,0x0F,0x04,
      (utc>>24)&0xff,(utc>>16)&0xff,(utc>>8)&0xff,utc&0xff,
      0x86,0x06,0x0A,0x01,0x01,0x0D,0x01,0x1E};
    uint8_t mac[4]; mac_for(0xD4,0,0,body,19,mac);
    uint8_t c[5+23]; c[0]=0x84;c[1]=0xD4;c[2]=0;c[3]=0;c[4]=23;
    memcpy(c+5,body,19);memcpy(c+24,mac,4);
    n=token2_process_apdu(c,28,resp,sizeof resp);
    if(sw(resp,n)!=0x9000){printf("config FAIL\n");fails++;} }

  /* re-auth (config cleared seed via restricted_sync) */
  { uint8_t c[]={0x80,0x4B,0x08,0x00,0x01,0x00};
    n=token2_process_apdu(c,sizeof c,resp,sizeof resp); memcpy(chal,resp,8); }
  { uint8_t blk[16],ct[16],c[5+16];
    memcpy(blk,chal,8);memset(blk+8,0,8);
    sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,blk,ct,1);
    c[0]=0x80;c[1]=0xCE;c[2]=0;c[3]=0;c[4]=0x10;memcpy(c+5,ct,16);
    token2_process_apdu(c,sizeof c,resp,sizeof resp); }

  /* 4. write the RFC6238 test seed "12345678901234567890" (20 bytes) */
  { const char* seed="12345678901234567890"; int L=20;
    uint8_t padded[32]; memcpy(padded,seed,L); padded[L]=0x80; memset(padded+L+1,0,32-L-1);
    uint8_t ct[32]; sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,padded,ct,2);
    uint8_t mac[4]; mac_for(0xC5,0x01,0x00,ct,32,mac);
    uint8_t c[5+36]; c[0]=0x84;c[1]=0xC5;c[2]=0x01;c[3]=0x00;c[4]=36;
    memcpy(c+5,ct,32);memcpy(c+37,mac,4);
    n=token2_process_apdu(c,41,resp,sizeof resp);
    if(sw(resp,n)!=0x9000){printf("seed FAIL\n");fails++;} }

  /* 5. read back params and generate the code the way the device display would */
  uint8_t s[63],sl=0,algo=1,step=0x1E;
  int ok = token2_get_totp_params(s,&sl,&algo,&step);
  if(!ok || sl!=20){printf("params FAIL (sl=%d)\n",sl);fails++;}
  /* at unix=59 with this seed, RFC6238 6-digit = 287082 */
  uint32_t code = totp_now_algo(s,sl,59,30,6,algo);
  printf("provisioned seed_len=%d algo=%d step=0x%02X\n",sl,algo,step);
  printf("TOTP at T=59 = %06u (expect 287082) %s\n", code, code==287082u?"OK":"FAIL");
  if(code!=287082u) fails++;

  printf("%s\n", fails?"PROVISION FLOW FAILED":"full provision flow OK (protocol over any transport)");
  return fails;
}
