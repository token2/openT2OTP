/* SPDX-License-Identifier: MIT
 * Copyright (c) 2020-2026 Token2 Sarl
 *
 * Part of openT2OTP - the open-source release of the firmware core that runs
 * Token2's programmable TOTP hardware tokens. See LICENSE and README.md.
 */
/* nfc_transport.c - glue between the contactless front-end and the token core.
 *
 * The host (token2_config.py) talks to the token as a raw ISO 7816 card over
 * PC/SC with NO applet SELECT. So the firmware must answer APDUs directly on
 * the basic logical channel after activation - do not require a SELECT AID.
 *
 * Two ways to deploy:
 *
 * A) Secure element running a JavaCard/GlobalPlatform applet
 *    - Implement process(APDU) and call token2_process_apdu() with the raw
 *      command buffer. Because the host sends no SELECT, register the applet
 *      as the default-selected applet on the card (GP INSTALL ... with the
 *      default-selected privilege) so activation routes APDUs straight to it.
 *    - Map SW words directly; the core already returns 9000/6983/6700.
 *
 * B) MCU + NFC frontend (e.g. ST25DV / PN7160 / RC663) presenting a
 *    Type-4 tag with an ISO-DEP (ISO 14443-4) card-emulation path
 *    - The frontend hands you deblocked I-blocks (APDUs). Feed each to
 *      token2_process_apdu(); return its output as the R-APDU.
 *    - Handle T=0 continuation (61 XX / 6C XX) only if your reader path is
 *      T=0; contactless ISO-DEP is T=1 / block based and needs none of that.
 */
#include "token2.h"
#include <string.h>

/* Call once at power-up / field activation. */
void nfc_on_activate(void){
    token2_init();
}

/* Call for each received command APDU. Returns R-APDU length. */
size_t nfc_on_apdu(const uint8_t *capdu, size_t clen,
                   uint8_t *rapdu, size_t rcap){
    return token2_process_apdu(capdu, clen, rapdu, rcap);
}
