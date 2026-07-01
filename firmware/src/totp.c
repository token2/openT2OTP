/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* totp.c - RFC 6238 TOTP over the stored seed, for on-device display.
 * Implements HMAC-SHA1 (algo=1) and HMAC-SHA256 (algo=2); the algorithm is
 * selected per the configured hmac_algo. Both are verified against the RFC 6238
 * Appendix B test vectors (see test/totp_kat.c).
 * The seed and config come from token2_state_t (written over the wire protocol). */
#include "totp.h"
#include "token2.h"
#include "token2_hal.h"
#include <string.h>

/* --- minimal SHA-1 --------------------------------------------------------*/
typedef struct { uint32_t h[5]; uint64_t len; uint8_t buf[64]; uint8_t n; } sha1_t;
static uint32_t rol(uint32_t v,int b){ return (v<<b)|(v>>(32-b)); }
static void sha1_blk(sha1_t*s,const uint8_t*p){
    uint32_t w[80],a,b,c,d,e,f,k,t; int i;
    for(i=0;i<16;i++) w[i]=(p[i*4]<<24)|(p[i*4+1]<<16)|(p[i*4+2]<<8)|p[i*4+3];
    for(i=16;i<80;i++) w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    a=s->h[0];b=s->h[1];c=s->h[2];d=s->h[3];e=s->h[4];
    for(i=0;i<80;i++){
        if(i<20){f=(b&c)|((~b)&d);k=0x5A827999;}
        else if(i<40){f=b^c^d;k=0x6ED9EBA1;}
        else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
        else{f=b^c^d;k=0xCA62C1D6;}
        t=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=t;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;
}
static void sha1_init(sha1_t*s){ s->h[0]=0x67452301;s->h[1]=0xEFCDAB89;s->h[2]=0x98BADCFE;s->h[3]=0x10325476;s->h[4]=0xC3D2E1F0;s->len=0;s->n=0; }
static void sha1_upd(sha1_t*s,const uint8_t*p,size_t l){ size_t i; for(i=0;i<l;i++){ s->buf[s->n++]=p[i]; s->len+=8; if(s->n==64){ sha1_blk(s,s->buf); s->n=0; } } }
static void sha1_fin(sha1_t*s,uint8_t out[20]){ uint64_t L=s->len; uint8_t pad=0x80; int i; sha1_upd(s,&pad,1); pad=0; while(s->n!=56) sha1_upd(s,&pad,1); for(i=7;i>=0;i--){ uint8_t b=(L>>(i*8))&0xff; sha1_upd(s,&b,1);} for(i=0;i<5;i++){ out[i*4]=s->h[i]>>24;out[i*4+1]=s->h[i]>>16;out[i*4+2]=s->h[i]>>8;out[i*4+3]=s->h[i]; } }

static void hmac_sha1(const uint8_t*key,size_t kl,const uint8_t*msg,size_t ml,uint8_t out[20]){
    uint8_t k[64],ip[64],op[64],ih[20]; size_t i; sha1_t s;
    memset(k,0,64);
    if(kl>64){ sha1_init(&s); sha1_upd(&s,key,kl); sha1_fin(&s,k); } else memcpy(k,key,kl);
    for(i=0;i<64;i++){ ip[i]=k[i]^0x36; op[i]=k[i]^0x5c; }
    sha1_init(&s); sha1_upd(&s,ip,64); sha1_upd(&s,msg,ml); sha1_fin(&s,ih);
    sha1_init(&s); sha1_upd(&s,op,64); sha1_upd(&s,ih,20); sha1_fin(&s,out);
}

/* --- SHA-256 --------------------------------------------------------------*/
typedef struct { uint32_t h[8]; uint64_t len; uint8_t buf[64]; uint8_t n; } sha256_t;
static uint32_t ror(uint32_t v,int b){ return (v>>b)|(v<<(32-b)); }
static const uint32_t K256[64] = {
 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
static void sha256_blk(sha256_t*s,const uint8_t*p){
    uint32_t w[64],a,b,c,d,e,f,g,h,t1,t2; int i;
    for(i=0;i<16;i++) w[i]=(p[i*4]<<24)|(p[i*4+1]<<16)|(p[i*4+2]<<8)|p[i*4+3];
    for(i=16;i<64;i++){
        uint32_t s0=ror(w[i-15],7)^ror(w[i-15],18)^(w[i-15]>>3);
        uint32_t s1=ror(w[i-2],17)^ror(w[i-2],19)^(w[i-2]>>10);
        w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    a=s->h[0];b=s->h[1];c=s->h[2];d=s->h[3];e=s->h[4];f=s->h[5];g=s->h[6];h=s->h[7];
    for(i=0;i<64;i++){
        uint32_t S1=ror(e,6)^ror(e,11)^ror(e,25);
        uint32_t ch=(e&f)^((~e)&g);
        t1=h+S1+ch+K256[i]+w[i];
        uint32_t S0=ror(a,2)^ror(a,13)^ror(a,22);
        uint32_t maj=(a&b)^(a&c)^(b&c);
        t2=S0+maj;
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void sha256_init(sha256_t*s){
    s->h[0]=0x6a09e667;s->h[1]=0xbb67ae85;s->h[2]=0x3c6ef372;s->h[3]=0xa54ff53a;
    s->h[4]=0x510e527f;s->h[5]=0x9b05688c;s->h[6]=0x1f83d9ab;s->h[7]=0x5be0cd19;
    s->len=0;s->n=0;
}
static void sha256_upd(sha256_t*s,const uint8_t*p,size_t l){ size_t i; for(i=0;i<l;i++){ s->buf[s->n++]=p[i]; s->len+=8; if(s->n==64){ sha256_blk(s,s->buf); s->n=0; } } }
static void sha256_fin(sha256_t*s,uint8_t out[32]){ uint64_t L=s->len; uint8_t pad=0x80; int i; sha256_upd(s,&pad,1); pad=0; while(s->n!=56) sha256_upd(s,&pad,1); for(i=7;i>=0;i--){ uint8_t b=(L>>(i*8))&0xff; sha256_upd(s,&b,1);} for(i=0;i<8;i++){ out[i*4]=s->h[i]>>24;out[i*4+1]=s->h[i]>>16;out[i*4+2]=s->h[i]>>8;out[i*4+3]=s->h[i]; } }

static void hmac_sha256(const uint8_t*key,size_t kl,const uint8_t*msg,size_t ml,uint8_t out[32]){
    uint8_t k[64],ip[64],op[64],ih[32]; size_t i; sha256_t s;
    memset(k,0,64);
    if(kl>64){ sha256_init(&s); sha256_upd(&s,key,kl); sha256_fin(&s,k); } else memcpy(k,key,kl);
    for(i=0;i<64;i++){ ip[i]=k[i]^0x36; op[i]=k[i]^0x5c; }
    sha256_init(&s); sha256_upd(&s,ip,64); sha256_upd(&s,msg,ml); sha256_fin(&s,ih);
    sha256_init(&s); sha256_upd(&s,op,64); sha256_upd(&s,ih,32); sha256_fin(&s,out);
}

/* RFC 6238 TOTP. algo: 1 = HMAC-SHA1 (20-byte MAC), 2 = HMAC-SHA256 (32-byte). */
uint32_t totp_now_algo(const uint8_t*seed,uint8_t seed_len,uint32_t unix_time,
                       uint32_t step,int digits,int algo){
    uint8_t msg[8],mac[32]; uint32_t counter=unix_time/step; int off,maclen; uint32_t bin,mod=1; int i;
    for(i=7;i>=0;i--){ msg[i]=counter&0xff; counter>>=8; }
    if(algo==2){ hmac_sha256(seed,seed_len,msg,8,mac); maclen=32; }
    else       { hmac_sha1  (seed,seed_len,msg,8,mac); maclen=20; }
    off=mac[maclen-1]&0x0f;   /* dynamic truncation offset = low nibble of last byte */
    bin=((mac[off]&0x7f)<<24)|(mac[off+1]<<16)|(mac[off+2]<<8)|mac[off+3];
    for(i=0;i<digits;i++) mod*=10;
    return bin%mod;
}

/* Back-compat wrapper: SHA1 (algo=1). */
uint32_t totp_now(const uint8_t*seed,uint8_t seed_len,uint32_t unix_time,uint32_t step,int digits){
    return totp_now_algo(seed,seed_len,unix_time,step,digits,1);
}
