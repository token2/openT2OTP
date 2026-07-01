/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* token2.c - device-side firmware for the Token2 single-profile programmable
 * TOTP token. Answers exactly the APDUs that token2_config.py sends.
 *
 * Command set (from PROTOCOL.md):
 *   Plain  (CLA 0x80): 0x41 get-info, 0x4B get-challenge, 0xCE auth-finish
 *   Secure (CLA 0x84): 0xC5 write-seed, 0xD4 write-config   (4-byte CBC-MAC)
 */
#include "token2.h"
#include "sm4.h"
#include "token2_hal.h"
#include <string.h>

/* device key: 8A D2 06 88 3C A3 69 48 2A B2 71 82 B6 E8 32 24
 * Public and identical on every device — this is intentional for the Token2
 * protocol (the value is documented). Where it lives doesn't matter because it
 * is not a secret. To make it a per-device SECRET for a fork, don't edit this
 * constant: source the key from the HAL (e.g. hal_get_device_key()) backed by
 * secure-element/protected-flash/PUF storage, and add retry throttling. See
 * docs/SECURITY.md ("The device key" and "Retry / lockout behaviour"). */
const uint8_t TOKEN2_DEVICE_SM4_KEY[16] = {
    0x8A,0xD2,0x06,0x88,0x3C,0xA3,0x69,0x48,
    0x2A,0xB2,0x71,0x82,0xB6,0xE8,0x32,0x24
};

/* NVM addresses */
#define NV_STATE_ADDR 0x0000

static token2_state_t S;

/* --- session (RAM only) auth state --- */
static uint8_t g_challenge[8];
static uint8_t g_have_challenge;
static uint8_t g_authed;

static void persist(void){ hal_nvm_write(NV_STATE_ADDR,&S,sizeof S); }

void token2_init(void){
    if(hal_nvm_read(NV_STATE_ADDR,&S,sizeof S)!=0 || S.serial_len==0){
        /* first boot / blank part: install factory defaults */
        memset(&S,0,sizeof S);
        /* OTPC-P2-i prefix 8659622 by default */
        memcpy(S.serial,"86596220000001",14); S.serial_len=14;
        S.hmac_algo=1; S.time_step=0x1E; S.display_timeout=1;
        S.restricted_sync=1;
        persist();
    }
}

/* Read the provisioned TOTP parameters, for on-device code generation/display.
 * Returns 1 if a seed is present, 0 otherwise. Never exposes the seed over the
 * wire protocol -- this is an internal accessor for the display path only.
 *
 * SECURE-ELEMENT NOTE: this default implementation copies the raw seed into the
 * caller's buffer (i.e. into MCU RAM). That is fine for a plain-MCU build, but on
 * a secure-element build it defeats the SE: the seed should never leave the SE.
 * For an SE build, do NOT return raw seed bytes here -- instead compute the HMAC
 * inside the SE and have the display path call an SE HMAC routine, so the seed is
 * never present in MCU memory. See docs/SECURITY.md ("Seed protection"). */
int token2_get_totp_params(uint8_t *seed, uint8_t *seed_len,
                           uint8_t *algo, uint8_t *step){
    if(seed && S.seed_len) memcpy(seed, S.seed, S.seed_len);
    if(seed_len) *seed_len = S.seed_len;
    if(algo)     *algo     = S.hmac_algo;
    if(step)     *step     = S.time_step;
    return S.seed_len ? 1 : 0;
}

/* helper: append SW to resp */
static size_t put_sw(uint8_t *resp, size_t n, uint16_t sw){
    resp[n]=(uint8_t)(sw>>8); resp[n+1]=(uint8_t)sw; return n+2;
}

/* ---- 0x41 get info -------------------------------------------------------
 * layout: 3 hdr | 1 serial_len | N serial | 2 sep | 4 utc BE  */
static size_t cmd_info(uint8_t *resp, size_t cap){
    size_t n=0; uint32_t t=hal_rtc_get();
    if(cap < (size_t)(3+1+S.serial_len+2+4+2)) return put_sw(resp,0,SW_WRONG_LEN);
    resp[n++]=0x00; resp[n++]=0x00; resp[n++]=0x00;        /* header (opaque) */
    resp[n++]=S.serial_len;
    memcpy(resp+n,S.serial,S.serial_len); n+=S.serial_len;
    resp[n++]=0x00; resp[n++]=0x00;                        /* separator */
    resp[n++]=t>>24; resp[n++]=t>>16; resp[n++]=t>>8; resp[n++]=t;
    return put_sw(resp,n,SW_OK);
}

/* ---- 0x4B get challenge --------------------------------------------------*/
static size_t cmd_challenge(uint8_t *resp, size_t cap){
    if(cap<10) return put_sw(resp,0,SW_WRONG_LEN);
    hal_rng(g_challenge,8);
    g_have_challenge=1; g_authed=0;
    memcpy(resp,g_challenge,8);
    return put_sw(resp,8,SW_OK);
}

/* ---- 0xCE auth finish ----------------------------------------------------
 * host sends SM4(challenge||8 zero bytes). Verify. */
static size_t cmd_auth_finish(const uint8_t *data, size_t dlen, uint8_t *resp){
    uint8_t plain[16], expect[16];
    if(S.key_locked)              return put_sw(resp,0,SW_AUTH_LOCK);
    if(!g_have_challenge||dlen!=16) return put_sw(resp,0,SW_DATA_NG);
    memcpy(plain,g_challenge,8);
    memset(plain+8,0,8);
    sm4_ecb_encrypt(TOKEN2_DEVICE_SM4_KEY,plain,expect,1);
    if(hal_ct_memcmp(expect,data,16)!=0){
        g_have_challenge=0;
        return put_sw(resp,0,SW_AUTH_LOCK);
    }
    g_authed=1; g_have_challenge=0;
    return put_sw(resp,0,SW_OK);
}

/* Verify the 4-byte MAC trailing a CLA 0x84 command.
 * MAC input = [0x80,INS,P1,P2,Lc'] || body, Lc'=body length (no MAC). */
static int mac_ok(uint8_t ins,uint8_t p1,uint8_t p2,
                  const uint8_t *body,size_t blen,const uint8_t *mac){
    uint8_t buf[256]; uint8_t got[4];
    if(blen+5>sizeof buf) return 0;
    buf[0]=0x80; buf[1]=ins; buf[2]=p1; buf[3]=p2; buf[4]=(uint8_t)blen;
    memcpy(buf+5,body,blen);
    sm4_cbc_mac(TOKEN2_DEVICE_SM4_KEY,buf,blen+5,got);
    return hal_ct_memcmp(got,mac,4)==0;
}

/* ---- 0xC5 write seed -----------------------------------------------------
 * body = SM4-ECB(seed with 9797-1 padding); trailing 4-byte MAC.
 * We decrypt to recover the seed then strip padding. */
static void sm4_ecb_decrypt_block(const uint8_t key[16],const uint8_t in[16],uint8_t out[16]){
    /* SM4 decryption = encryption with reversed round keys */
    uint32_t rk[32],rrk[32]; int i;
    sm4_key_schedule(key,rk);
    for(i=0;i<32;i++) rrk[i]=rk[31-i];
    sm4_ecb_block(rrk,in,out);
}

static size_t cmd_seed(const uint8_t *body,size_t blen,uint8_t *resp){
    uint8_t pt[64]; size_t i,n; int slen;
    if(!g_authed) return put_sw(resp,0,SW_AUTH_LOCK);
    /* ciphertext is the padded seed: 1..63-byte seed -> 16..64-byte ciphertext.
     * (a 63-byte seed pads to 64; the 32-byte form pads to 48.) */
    if(blen==0 || blen%16!=0 || blen>64) return put_sw(resp,0,SW_DATA_NG);
    for(i=0;i<blen;i+=16) sm4_ecb_decrypt_block(TOKEN2_DEVICE_SM4_KEY,body+i,pt+i);
    n=blen;
    /* strip 9797-1 method-2 padding: the pad marker is the rightmost 0x80,
     * with only 0x00 after it. Scan from the end. */
    slen=-1;
    for(i=n;i>0;i--){ if(pt[i-1]==0x80){ slen=(int)(i-1); break; } if(pt[i-1]!=0x00) break; }
    if(slen<0||slen>63) return put_sw(resp,0,SW_DATA_NG);
    /* wipe the whole stored buffer before writing, so a shorter new seed leaves
     * no stale tail bytes from a previous, longer seed. */
    memset(S.seed,0,sizeof S.seed);
    memcpy(S.seed,pt,(size_t)slen); S.seed_len=(uint8_t)slen;
    persist();
    /* scrub the decrypted plaintext from the stack; it held the raw seed. */
    memset(pt,0,sizeof pt);
    return put_sw(resp,0,SW_OK);
}

/* ---- 0xD4 write config ---------------------------------------------------
 * body = 19-byte plaintext TLV; trailing 4-byte MAC.
 * 81 11  1F 01 <disp>  0F 04 <utc BE>  86 06  0A 01 <algo>  0D 01 <step> */
static size_t cmd_config(const uint8_t *body,size_t blen,uint8_t *resp){
    uint32_t utc;
    if(!g_authed) return put_sw(resp,0,SW_AUTH_LOCK);
    if(blen!=19) return put_sw(resp,0,SW_DATA_NG);
    if(body[0]!=0x81||body[1]!=0x11) return put_sw(resp,0,SW_DATA_NG);
    if(body[2]!=0x1F||body[3]!=0x01) return put_sw(resp,0,SW_DATA_NG);
    S.display_timeout=body[4];
    if(body[5]!=0x0F||body[6]!=0x04) return put_sw(resp,0,SW_DATA_NG);
    utc=((uint32_t)body[7]<<24)|((uint32_t)body[8]<<16)|((uint32_t)body[9]<<8)|body[10];
    if(body[11]!=0x86||body[12]!=0x06) return put_sw(resp,0,SW_DATA_NG);
    if(body[13]!=0x0A||body[14]!=0x01) return put_sw(resp,0,SW_DATA_NG);
    S.hmac_algo=body[15];
    if(body[16]!=0x0D||body[17]!=0x01) return put_sw(resp,0,SW_DATA_NG);
    S.time_step=body[18];
    hal_rtc_set(utc);
    if(S.restricted_sync){ S.seed_len=0; memset(S.seed,0,sizeof S.seed); }
    persist();
    return put_sw(resp,0,SW_OK);
}

/* ---- dispatcher ----------------------------------------------------------*/
size_t token2_process_apdu(const uint8_t *a, size_t alen,
                           uint8_t *resp, size_t cap){
    uint8_t cla,ins,p1,p2,lc; const uint8_t *data;
    if(alen<4) return put_sw(resp,0,SW_WRONG_LEN);
    cla=a[0]; ins=a[1]; p1=a[2]; p2=a[3];
    lc = (alen>=5)?a[4]:0;
    data = (alen>=6)?a+5:NULL;
    if(alen>=5 && alen < (size_t)(5+lc)) return put_sw(resp,0,SW_WRONG_LEN);

    if(cla==0x80){
        if(ins==0x41) return cmd_info(resp,cap);
        if(ins==0x4B && p1==0x08) return cmd_challenge(resp,cap);
        if(ins==0xCE) return cmd_auth_finish(data,lc,resp);
        return put_sw(resp,0,SW_INS_NG);
    }
    if(cla==0x84){
        /* split body|mac: last 4 bytes are MAC */
        if(lc<4) return put_sw(resp,0,SW_WRONG_LEN);
        {
            size_t blen=lc-4; const uint8_t *body=data; const uint8_t *mac=data+blen;
            if(!mac_ok(ins,p1,p2,body,blen,mac)) return put_sw(resp,0,SW_AUTH_LOCK);
            if(ins==0xC5 && p1==0x01) return cmd_seed(body,blen,resp);
            if(ins==0xD4)             return cmd_config(body,blen,resp);
            return put_sw(resp,0,SW_INS_NG);
        }
    }
    return put_sw(resp,0,SW_CLA_NG);
}
