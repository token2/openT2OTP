#!/usr/bin/env python3
# SPDX-License-Identifier: CERN-OHL-P-2.0
# Copyright (c) 2020-2026 Token2 Sarl
#
# Part of openT2OTP hardware reference. Licensed under CERN-OHL-P v2.
# See hardware/LICENSE-HARDWARE.
"""Emit a fabricable board-outline Gerber (RS-274X) and an Excellon drill file
for the openT2OTP board, so a fab can cut the correct 48x24mm rounded outline
with 4 M2 mounting holes at the OT1 enclosure boss positions.

SCOPE: this exports the mechanical layers that ARE fully defined -- the board
edge and the mounting holes. It does NOT export copper layers, because the board
is not routed (see gen_pcb.py). A fab can produce the correctly-shaped, drilled
blank from these; the copper Gerbers come after you route the board in KiCad.

Usage:
  python3 gen_gerber.py            # writes gerbers/ directory
"""
import os, math

BW,BH,CORNER = 48.0,24.0,3.0
HW,HH = BW/2,BH/2
MOUNT=[(-19.2,7.3),(19.2,7.3),(19.2,-7.3),(-19.2,-7.3)]
MOUNT_DRILL=2.2
OUTDIR="gerbers"

def gerber_outline():
    """RS-274X board outline: rounded rectangle + edge notches. Units mm.
    Traversed clockwise; convex corners are G02 (CW) quarter-arcs."""
    L=[]
    L.append("%FSLAX46Y46*%")          # 4 integer, 6 decimal
    L.append("%MOMM*%")                 # millimetres
    L.append("G01*")
    L.append("%ADD10C,0.100000*%")      # 0.1mm round aperture
    L.append("D10*")
    L.append("G75*")                    # multi-quadrant arc mode
    def xy(x,y): return f"X{int(round(x*1e6)):d}Y{int(round(y*1e6)):d}"
    def ij(i,j): return f"I{int(round(i*1e6)):d}J{int(round(j*1e6)):d}"
    r=CORNER; L_,R_,T_,B_=-HW,HW,HH,-HH
    # Clockwise from top-left corner-start. Explicit G01 before straights and G02
    # before each arc so every viewer interprets the mode unambiguously.
    L.append(f"G01*")
    L.append(f"{xy(L_+r,T_)}D02*")
    L.append(f"{xy(R_-r,T_)}D01*")                                  # top edge
    L.append(f"G02{xy(R_,T_-r)}{ij(0,-r)}D01*"); L.append("G01*")   # TR corner
    L.append(f"{xy(R_,B_+r)}D01*")                                  # right edge
    L.append(f"G02{xy(R_-r,B_)}{ij(-r,0)}D01*"); L.append("G01*")   # BR corner
    L.append(f"{xy(L_+r,B_)}D01*")                                  # bottom edge
    L.append(f"G02{xy(L_,B_+r)}{ij(0,r)}D01*"); L.append("G01*")    # BL corner
    L.append(f"{xy(L_,T_-r)}D01*")                                  # left edge
    L.append(f"G02{xy(L_+r,T_)}{ij(r,0)}D01*"); L.append("G01*")    # TL corner
    # --- edge notches: small rectangular bite into top & bottom edges ---
    nw,nd=2.0,1.0
    for nx in (-8.0,8.0):
        for edge_y,depth in ((T_,-nd),(B_,nd)):
            x1,x2=nx-nw/2,nx+nw/2; y0=edge_y; y1=edge_y+depth
            pts=[(x1,y0),(x1,y1),(x2,y1),(x2,y0)]
            L.append(f"{xy(*pts[0])}D02*")
            for p in pts[1:]: L.append(f"{xy(*p)}D01*")
    L.append("M02*")
    return "\n".join(L)

def excellon_drill():
    L=[]
    L.append("M48")
    L.append("METRIC,TZ")
    L.append("T1C%.3f"%MOUNT_DRILL)
    L.append("%")
    L.append("T1")
    for x,y in MOUNT:
        L.append("X%.3fY%.3f"%(x,y))
    L.append("M30")
    return "\n".join(L)

def main():
    os.makedirs(OUTDIR,exist_ok=True)
    with open(os.path.join(OUTDIR,"openT2OTP-Edge_Cuts.gbr"),"w") as f:
        f.write(gerber_outline()+"\n")
    with open(os.path.join(OUTDIR,"openT2OTP-PTH.drl"),"w") as f:
        f.write(excellon_drill()+"\n")
    # a README so nobody mistakes this for a full fab package
    with open(os.path.join(OUTDIR,"README.md"),"w") as f:
        f.write(
"# Gerbers — board outline + drill only\n\n"
"These files define the **mechanical** board only:\n\n"
"- `openT2OTP-Edge_Cuts.gbr` — the 48 x 24 mm rounded board outline (RS-274X).\n"
"- `openT2OTP-PTH.drl` — Excellon drill: 4 x M2 (2.2 mm) mounting holes at the\n"
"  OT1 enclosure boss positions (+/-19.2, +/-7.3 mm).\n\n"
"They let a fab cut and drill the correctly shaped blank that fits the OT1 case.\n"
"**Copper layers are intentionally absent** — the board is not yet routed. Route\n"
"it in KiCad from `../openT2OTP.kicad_pcb`, run DRC, then export the full Gerber\n"
"set (copper, soldermask, silk, paste) for production.\n")
    print("wrote", OUTDIR+"/openT2OTP-Edge_Cuts.gbr, openT2OTP-PTH.drl, README.md")

if __name__=="__main__":
    main()
