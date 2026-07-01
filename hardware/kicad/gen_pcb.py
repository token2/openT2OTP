#!/usr/bin/env python3
# SPDX-License-Identifier: CERN-OHL-P-2.0
# Copyright (c) 2020-2026 Token2 Sarl
#
# Part of openT2OTP hardware reference. Licensed under CERN-OHL-P v2 (permissive).
# See hardware/LICENSE-HARDWARE.
"""Generate a KiCad 7 PCB (.kicad_pcb) for the openT2OTP reference board, sized
to fit the OT1 enclosure (from ot1.stp).

Enclosure-derived geometry (measured from the STEP assembly):
  - internal cavity footprint ~ 50.4 x 26.0 mm
  - four corner screw bosses at (+/-19.2, +/-7.3) mm from center
So the board is 48 x 24 mm (1.2 mm clearance per side) with 4 M2 mounting holes
at the boss centers.

SCOPE / HONESTY: this produces a real board OUTLINE, MOUNTING HOLES, and component
PLACEMENT with courtyards -- a correct mechanical/placement starting point that
fits the case. It is NOT auto-routed: copper traces, the ground pour, and the
13.56 MHz antenna matching must be routed and DRC-checked in KiCad by a person.
The footprints use generic pads sized for each part; verify against your exact MPNs
before fabrication. Treat this as the placement baseline, not a fab-ready layout.

Usage:  python3 gen_pcb.py > openT2OTP.kicad_pcb
"""

import sys, uuid

def u():  # fresh uuid for each kicad item
    return str(uuid.uuid4())

# ---- board geometry (mm), origin at board center ----------------------------
BW, BH = 48.0, 24.0                 # board width/height
HW, HH = BW/2.0, BH/2.0
CORNER = 3.0                        # outline corner radius
MOUNT = [(-19.2, 7.3), (19.2, 7.3), (19.2, -7.3), (-19.2, -7.3)]
MOUNT_D = 2.2                       # M2 clearance hole

# KiCad uses a top-left page origin; place the board around (150,100) on-sheet.
OX, OY = 150.0, 100.0
def X(x): return OX + x
def Y(y): return OY - y             # flip Y so +y is up, like the STEP

# ---- components: ref, value, (x,y), rotation, footprint-kind ----------------
# placement reasoning:
#  DS1 6-digit LCD  -> centered, top side, it dominates the face
#  BT1 CR2032       -> back side, centered low
#  U1 MCU (QFN)     -> under the LCD area, center
#  U2 NFC front-end -> near the antenna feed (right edge)
#  U3 SE050         -> beside the MCU
#  L1 antenna loop  -> perimeter (drawn as edge trace placeholder on F.Cu)
#  C1,C2 tuning     -> at the antenna feed
#  Y1 32.768kHz     -> by the MCU
#  SW1 button       -> bottom edge, user-accessible
#  J1 SWD           -> edge, for programming
COMPS = [
    # --- FRONT (display face) --------------------------------------------------
    ("DS1","6-digit LCD (COG)",  9.0,  0.0, 0, "lcd",    "F"),
    ("SW1","Power/show",        -9.5,  0.0, 0, "sw_th",  "F"),  # through-hole dome
    # --- BACK (electronics), viewed from back -----------------------------------
    # Battery clip at the LEFT end (edge-mounted, overhangs the edge).
    ("BT1","CR2025/2032 clip", -14.0,  0.0, 0, "bat_clip","B"),
    # IC cluster spread across the RIGHT half, matching the photo arrangement:
    ("Y1", "32.768kHz",          1.0,  6.5, 0, "xtal_can","B"), # crystal can, top-left of cluster
    ("U2", "NFC front-end",     13.0,  5.5, 0, "qfn32", "B"),   # larger QFN, upper-right
    ("U1", "MCU (QFN)",          4.0, -3.0, 0, "qfn48", "B"),   # MCU, center
    ("U3", "Secure element",    18.5, -2.0, 0, "son8",  "B"),   # SOIC-8, far right
    # tuning + passives tucked around them
    ("C1", "tune C0G",          -5.5,  8.0, 90,"c0805", "B"),
    ("C2", "tune C0G",          -3.0,  8.0, 90,"c0805", "B"),
    ("C3", "LCD bias",          20.0,  6.0, 0, "c0805", "B"),
    ("C7", "100nF",             10.0, -3.0, 0, "c0603", "B"),
    ("C9", "1uF",               10.0, -6.5, 0, "c0603", "B"),
    ("R1", "10k",                0.0, -9.0, 0, "c0603", "B"),
    ("R2", "10k",                3.5, -9.0, 0, "c0603", "B"),
    ("J1", "prog pads",         18.0, -8.5, 0, "swd",   "B"),   # test/prog pads
]

# footprint kind -> (body_w, body_h, courtyard pad list). Pads are simplified:
# we emit a labeled rectangle on F.Fab, a courtyard, a reference, and generic
# SMD pads so the part has real copper to route to.
KIND = {
    "lcd":      (26.0, 15.0),   # COG LCD module, right ~60% of the face
    "qfn48":    (7.0, 7.0),
    "qfn32":    (6.0, 6.0),     # NFC front-end, the larger QFN in the photo
    "son8":     (5.0, 4.0),     # SOIC-8 secure element
    "cr2032":   (20.0, 20.0),
    "bat_clip": (18.0, 16.0),   # edge SMT coin-cell clip (overhangs edge)
    "xtal":     (3.2, 1.5),
    "xtal_can": (8.0, 3.5),     # cylindrical 32.768kHz can
    "c0805":    (2.0, 1.25),
    "c0603":    (1.6, 0.8),
    "sw":       (4.0, 3.0),
    "sw_th":    (6.0, 6.0),     # through-hole tactile/dome button
    "swd":      (5.0, 6.0),
}

def rounded_outline():
    """Edge.Cuts as 4 lines + 4 arcs (rounded rectangle)."""
    r=CORNER; out=[]
    # corner centers
    L,R,T,B = -HW, HW, HH, -HH
    # straight segments (inset by r at the rounded corners)
    segs=[
        ((L+r,T),(R-r,T)),   # top
        ((R,T-r),(R,B+r)),   # right
        ((R-r,B),(L+r,B)),   # bottom
        ((L,B+r),(L,T-r)),   # left
    ]
    for (x1,y1),(x2,y2) in segs:
        out.append(f'  (gr_line (start {X(x1):.3f} {Y(y1):.3f}) (end {X(x2):.3f} {Y(y2):.3f}) (stroke (width 0.12) (type solid)) (layer "Edge.Cuts") (uuid {u()}))')
    # arcs at each corner (start, mid, end)
    import math
    corners=[(L+r,T-r,90,180),(R-r,T-r,0,90),(R-r,B+r,270,360),(L+r,B+r,180,270)]
    for cx,cy,a0,a1 in corners:
        am=math.radians((a0+a1)/2); a0r=math.radians(a0); a1r=math.radians(a1)
        sx,sy=cx+r*math.cos(a0r), cy+r*math.sin(a0r)
        mx,my=cx+r*math.cos(am),  cy+r*math.sin(am)
        ex,ey=cx+r*math.cos(a1r), cy+r*math.sin(a1r)
        out.append(f'  (gr_arc (start {X(sx):.3f} {Y(sy):.3f}) (mid {X(mx):.3f} {Y(my):.3f}) (end {X(ex):.3f} {Y(ey):.3f}) (stroke (width 0.12) (type solid)) (layer "Edge.Cuts") (uuid {u()}))')
    return "\n".join(out)

def mounting_holes():
    out=[]
    for i,(mx,my) in enumerate(MOUNT,1):
        out.append(f'''  (footprint "MountingHole" (layer "F.Cu") (uuid {u()})
    (at {X(mx):.3f} {Y(my):.3f})
    (property "Reference" "H{i}" (at 0 -2.6 0) (layer "F.SilkS") (uuid {u()}) (effects (font (size 0.8 0.8) (thickness 0.12))))
    (property "Value" "M2" (at 0 2.6 0) (layer "F.Fab") (uuid {u()}) (effects (font (size 0.8 0.8) (thickness 0.12))))
    (pad "" thru_hole circle (at 0 0) (size {MOUNT_D+1.4:.2f} {MOUNT_D+1.4:.2f}) (drill {MOUNT_D:.2f}) (layers "*.Cu" "*.Mask") (uuid {u()}))
  )''')
    return "\n".join(out)

def antenna_loop():
    """NFC antenna L1: a perimeter loop trace on F.Cu, 2.0mm inside the edge,
    3 turns hinted by concentric rounded rects. Placeholder geometry -- the real
    inductance must be measured and the loop retuned (see antenna/tune.py)."""
    out=[]
    inset0=2.0; pitch=0.6; turns=3; wid=0.5
    import math
    for t in range(turns):
        ins=inset0+t*pitch
        l,r,tp,b=-HW+ins,HW-ins,HH-ins,-HH+ins
        pts=[(l+CORNER,tp),(r-CORNER,tp),(r,tp-CORNER),(r,b+CORNER),
             (r-CORNER,b),(l+CORNER,b),(l,b+CORNER),(l,tp-CORNER),(l+CORNER,tp)]
        for (x1,y1),(x2,y2) in zip(pts,pts[1:]):
            out.append(f'  (segment (start {X(x1):.3f} {Y(y1):.3f}) (end {X(x2):.3f} {Y(y2):.3f}) (width {wid}) (layer "F.Cu") (net 0) (uuid {u()}))')
    return "\n".join(out)

def edge_notches():
    """The real board has small rectangular retention notches on its long edges
    (visible in the photos). Draw them as short Edge.Cuts rectangles cut into the
    top and bottom edges at +/-8 mm from center."""
    out=[]
    nw,nd=2.0,1.0   # notch width along edge, depth into board
    for nx in (-8.0, 8.0):
        for edge_y,depth in ((HH,-nd),(-HH,nd)):  # top edge cuts down, bottom up
            x1,x2=nx-nw/2,nx+nw/2
            y0=edge_y; y1=edge_y+depth
            pts=[(x1,y0),(x1,y1),(x2,y1),(x2,y0)]
            for (ax,ay),(bx,by) in zip(pts,pts[1:]):
                out.append(f'  (gr_line (start {X(ax):.3f} {Y(ay):.3f}) (end {X(bx):.3f} {Y(by):.3f}) (stroke (width 0.12) (type solid)) (layer "Edge.Cuts") (uuid {u()}))')
    return "\n".join(out)

def resolve_overlaps(comps, iters=200):
    """Nudge back-side parts apart until no courtyards overlap and all stay on
    board. Keeps the photo-matched arrangement but relaxes exact positions.
    Fixed anchors (big parts) don't move; small parts drift to clear space."""
    cy=1.0
    fixed={"DS1","SW1","BT1"}   # only physically-fixed big parts anchored;
                                 # ICs may nudge slightly so everything clears
    pos={c[0]:[c[2],c[3]] for c in comps}
    kinds={c[0]:c[5] for c in comps}
    sides={c[0]:c[6] for c in comps}
    def halfw(ref): w,h=KIND[kinds[ref]]; return w/2+cy, h/2+cy
    for _ in range(iters):
        moved=False
        refs=[c[0] for c in comps]
        for i in range(len(refs)):
            for j in range(i+1,len(refs)):
                a,b=refs[i],refs[j]
                if sides[a]!=sides[b]: continue
                hwa,hha=halfw(a); hwb,hhb=halfw(b)
                dx=pos[b][0]-pos[a][0]; dy=pos[b][1]-pos[a][1]
                ox=(hwa+hwb)-abs(dx); oy=(hha+hhb)-abs(dy)
                if ox>0.05 and oy>0.05:   # overlapping
                    moved=True
                    # push apart along the smaller-overlap axis
                    if ox<oy:
                        push=(ox/2+0.05)*(1 if dx>=0 else -1)
                        if a not in fixed: pos[a][0]-=push
                        if b not in fixed: pos[b][0]+=push
                    else:
                        push=(oy/2+0.05)*(1 if dy>=0 else -1)
                        if a not in fixed: pos[a][1]-=push
                        if b not in fixed: pos[b][1]+=push
        # keep small parts on-board (battery may overhang)
        for ref in refs:
            if ref in ("BT1",): continue
            hw_,hh_=halfw(ref)
            pos[ref][0]=max(-HW+hw_, min(HW-hw_, pos[ref][0]))
            pos[ref][1]=max(-HH+hh_, min(HH-hh_, pos[ref][1]))
        if not moved: break
    return [(c[0],c[1],round(pos[c[0]][0],2),round(pos[c[0]][1],2),c[4],c[5],c[6]) for c in comps]


def component(ref,val,x,y,rot,kind,side="F"):
    w,h=KIND[kind]
    cy=1.0  # courtyard margin
    S=side  # "F" or "B"
    layer=f"{S}.Cu"
    slk=f"{S}.SilkS"; fab=f"{S}.Fab"; crt=f"{S}.CrtYd"
    paste=f"{S}.Paste"; mask=f"{S}.Mask"
    tag = "" if S=="F" else "  (back side)"
    fp=[f'  (footprint "openT2OTP:{kind}" (layer "{layer}") (uuid {u()})',
        f'    (at {X(x):.3f} {Y(y):.3f} {rot})',
        f'    (property "Reference" "{ref}" (at 0 {-(h/2+1.4):.2f} {rot}) (layer "{slk}") (uuid {u()}) (effects (font (size 0.8 0.8) (thickness 0.12)){" (justify mirror)" if S=="B" else ""}))',
        f'    (property "Value" "{val}{tag}" (at 0 {(h/2+1.4):.2f} {rot}) (layer "{fab}") (uuid {u()}) (effects (font (size 0.6 0.6) (thickness 0.1)){" (justify mirror)" if S=="B" else ""}))',
        f'    (fp_rect (start {-w/2:.2f} {-h/2:.2f}) (end {w/2:.2f} {h/2:.2f}) (stroke (width 0.1) (type default)) (fill none) (layer "{fab}") (uuid {u()}))',
        f'    (fp_rect (start {-(w/2+cy):.2f} {-(h/2+cy):.2f}) (end {w/2+cy:.2f} {h/2+cy:.2f}) (stroke (width 0.05) (type default)) (fill none) (layer "{crt}") (uuid {u()}))',
        f'    (fp_rect (start {-w/2:.2f} {-h/2:.2f}) (end {w/2:.2f} {h/2:.2f}) (stroke (width 0.12) (type default)) (fill none) (layer "{slk}") (uuid {u()}))',
    ]
    pad_w=min(1.2,w/3); pad_h=min(1.0,h/3)
    fp.append(f'    (pad "1" smd rect (at {-w/2+pad_w:.2f} {-h/2+pad_h:.2f}) (size {pad_w:.2f} {pad_h:.2f}) (layers "{layer}" "{paste}" "{mask}") (uuid {u()}))')
    fp.append(f'    (pad "2" smd rect (at {w/2-pad_w:.2f} {h/2-pad_h:.2f}) (size {pad_w:.2f} {pad_h:.2f}) (layers "{layer}" "{paste}" "{mask}") (uuid {u()}))')
    fp.append('  )')
    return "\n".join(fp)

def main():
    placed=resolve_overlaps(COMPS)
    parts="\n".join(component(*c) for c in placed)
    body=f'''(kicad_pcb (version 20221018) (generator gen_pcb.py)
  (general (thickness 1.6))
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (34 "B.SilkS" user "B.Silkscreen")
    (35 "F.SilkS" user "F.Silkscreen")
    (36 "B.Mask" user)
    (37 "F.Mask" user)
    (38 "B.Fab" user)
    (39 "F.Fab" user)
    (44 "Edge.Cuts" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
  )
  (setup (pad_to_mask_clearance 0))
  (net 0 "")

{rounded_outline()}

{edge_notches()}

{mounting_holes()}

  (gr_text "openT2OTP  48x24mm  fits OT1 enclosure" (at {X(0):.3f} {Y(-HH+1.5):.3f}) (layer "F.SilkS") (uuid {u()}) (effects (font (size 1.0 1.0) (thickness 0.15))))

{antenna_loop()}

{parts}
)
'''
    sys.stdout.write(body)

if __name__=="__main__":
    main()
