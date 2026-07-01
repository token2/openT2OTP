#!/usr/bin/env python3
# SPDX-License-Identifier: CERN-OHL-P-2.0
# Copyright (c) 2020-2026 Token2 Sarl
#
# Part of openT2OTP hardware reference. Licensed under CERN-OHL-P v2 (permissive).
# See hardware/LICENSE-HARDWARE.
"""NFC antenna tuning helper for the openT2OTP reference board.

The token's antenna is a PCB copper loop that must resonate at 13.56 MHz.
The loop inductance L, plus the matching capacitance C, forms a parallel tank:

    f = 1 / (2*pi*sqrt(L*C))   ->   C = 1 / ((2*pi*f)^2 * L)

Workflow:
  1. Lay out the coil (see antenna/coil.md for the reference geometry).
  2. Measure its inductance L with an LCR meter / VNA on a coupon.
  3. Run this script with the measured L to get the target total C.
  4. Split C across C1/C2 (two caps, one per antenna terminal -> series),
     so each cap is ~2*C. Use C0G/NP0 parts.
  5. Trim on the bench: sweep with a VNA for the resonance dip, or maximise
     read range against your target reader.

Usage:
    python3 tune.py --inductance 2.5e-6
    python3 tune.py -L 2.5u -f 13.56M
"""
import argparse
import math

def parse_si(s):
    """Parse values like 2.5u, 13.56M, 47p, 1e-6."""
    s = str(s).strip()
    mult = {'p':1e-12,'n':1e-9,'u':1e-6,'m':1e-3,'k':1e3,'M':1e6,'G':1e9}
    if s and s[-1] in mult:
        return float(s[:-1]) * mult[s[-1]]
    return float(s)

def total_cap(L, f):
    return 1.0 / ((2*math.pi*f)**2 * L)

def fmt(c):
    if c >= 1e-9: return f"{c*1e9:.1f} nF"
    return f"{c*1e12:.1f} pF"

def main():
    ap = argparse.ArgumentParser(description="NFC antenna tuning capacitor calculator")
    ap.add_argument("-L","--inductance", required=True,
                    help="measured loop inductance, e.g. 2.5u or 2.5e-6 (henries)")
    ap.add_argument("-f","--frequency", default="13.56M",
                    help="target resonance (default 13.56M for ISO 14443)")
    ap.add_argument("--q", type=float, default=None,
                    help="optional loaded Q to estimate bandwidth")
    args = ap.parse_args()

    L = parse_si(args.inductance)
    f = parse_si(args.frequency)
    C = total_cap(L, f)

    print(f"Target frequency     : {f/1e6:.3f} MHz")
    print(f"Measured inductance  : {L*1e6:.3f} uH")
    print(f"Total tank capacitance: {fmt(C)}")
    print(f"  -> symmetric split : C1 = C2 = {fmt(2*C)}  (two caps in series)")
    print(f"  -> single cap       : C = {fmt(C)}")
    print("Use C0G/NP0 dielectric. Nearest E12 value, then trim on the bench.")
    if args.q:
        bw = f/args.q
        print(f"Estimated bandwidth  : {bw/1e3:.0f} kHz (at Q={args.q:g})")

if __name__ == "__main__":
    main()
