/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
#include "totp.h"
#include <stdio.h>
#include <string.h>
int main(void){
  /* RFC 6238 Appendix B test vectors.
     SHA1 seed  = ASCII "12345678901234567890"                 (20 bytes)
     SHA256 seed= ASCII "12345678901234567890123456789012"     (32 bytes) */
  const char* s1="12345678901234567890";
  const char* s2="12345678901234567890123456789012";
  int fails=0;

  /* --- SHA1 (algo=1) --- */
  uint32_t c8 = totp_now((const uint8_t*)s1,20,59,30,8);
  uint32_t c6 = totp_now((const uint8_t*)s1,20,59,30,6);
  uint32_t d8 = totp_now((const uint8_t*)s1,20,1111111109,30,8);
  printf("SHA1   T=59 8-digit=%08u (expect 94287082)  6-digit=%06u (expect 287082)\n",c8,c6);
  printf("SHA1   T=1111111109 8-digit=%08u (expect 07081804)\n",d8);
  if(!(c8==94287082u && c6==287082u && d8==7081804u)) fails++;

  /* --- SHA256 (algo=2), RFC 6238 8-digit vectors --- */
  uint32_t e59  = totp_now_algo((const uint8_t*)s2,32,59,        30,8,2);
  uint32_t e148 = totp_now_algo((const uint8_t*)s2,32,1111111109,30,8,2);
  uint32_t e200 = totp_now_algo((const uint8_t*)s2,32,2000000000,30,8,2);
  printf("SHA256 T=59=%08u (expect 46119246)  T=1111111109=%08u (expect 68084774)\n",e59,e148);
  printf("SHA256 T=2000000000=%08u (expect 90698825)\n",e200);
  if(!(e59==46119246u && e148==68084774u && e200==90698825u)) fails++;

  printf("%s\n", fails?"TOTP RFC6238 KAT: FAIL":"TOTP RFC6238 KAT: PASS");
  return fails?1:0;
}
