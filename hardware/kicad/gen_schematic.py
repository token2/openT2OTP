#!/usr/bin/env python3
# SPDX-License-Identifier: CERN-OHL-P-2.0
# Copyright (c) 2020-2026 Token2 Sarl
#
# Part of openT2OTP hardware reference. Licensed under CERN-OHL-P v2 (permissive).
# See hardware/LICENSE-HARDWARE.
"""Generate a self-contained KiCad 7 schematic (.kicad_sch) for the openT2OTP
reference board.

This produces a *connectivity* schematic: every component is placed as a labeled
box with its pins, and nets are expressed with global labels so the wiring is
unambiguous without hand-routing wire segments. It opens directly in KiCad 7+
with no external symbol libraries required (symbols are embedded).

Run:  python3 gen_schematic.py > openT2OTP.kicad_sch

The intent is that a contributor imports this, replaces the generic boxes with
real library footprints for their exact parts, and routes the PCB. The netlist
(nets below) is the source of truth; see ../PINMAP.md.
"""
import uuid

def U(): return str(uuid.uuid4())

# ---- component definition: (ref, value, [pin_name...]) -----------------------
COMPONENTS = [
    ("U1", "SAM L22 (ATSAML22N18A)",
     ["VDD","GND","XIN32","XOUT32",
      "LCD_COM0","LCD_COM1","LCD_COM2","LCD_COM3",
      "LCD_SEG0","LCD_SEG10","CAPL","CAPH",
      "NFC_SCK","NFC_MOSI","NFC_MISO","NFC_CS","NFC_IRQ",
      "SE_SDA","SE_SCL","BTN","SWDIO","SWCLK","RESET"]),
    ("DS1", "6-digit 7-seg LCD",
     ["COM0","COM1","COM2","COM3","SEG0","SEG10"]),
    ("U2", "NFC front-end (ST25R3916)",
     ["VDD","GND","SCLK","MOSI","MISO","CS","IRQ","ANT1","ANT2"]),
    ("U3", "Secure element (SE050)",
     ["VDD","GND","SDA","SCL"]),
    ("Y1", "32.768 kHz", ["1","2"]),
    ("L1", "NFC antenna loop", ["A","B"]),
    ("C1", "tune (C0G)", ["1","2"]),
    ("C2", "tune (C0G)", ["1","2"]),
    ("C3", "LCD bias", ["1","2"]),
    ("C7", "100nF", ["1","2"]),
    ("C9", "1uF", ["1","2"]),
    ("R1", "10k", ["1","2"]),
    ("R2", "10k", ["1","2"]),
    ("SW1", "Power/show", ["1","2"]),
    ("BT1", "CR2032", ["+","-"]),
    ("J1", "SWD", ["SWDIO","SWCLK","RESET","VDD","GND"]),
]

# ---- pin -> net map (the electrical truth) -----------------------------------
NETS = {
    "VDD":   [("U1","VDD"),("U2","VDD"),("U3","VDD"),("C7","1"),("C9","1"),
              ("R1","1"),("R2","1"),("BT1","+"),("J1","VDD")],
    "GND":   [("U1","GND"),("U2","GND"),("U3","GND"),("C7","2"),("C9","2"),
              ("SW1","2"),("BT1","-"),("J1","GND")],
    "XIN32":  [("U1","XIN32"),("Y1","1")],
    "XOUT32": [("U1","XOUT32"),("Y1","2")],
    "LCD_COM0":[("U1","LCD_COM0"),("DS1","COM0")],
    "LCD_COM1":[("U1","LCD_COM1"),("DS1","COM1")],
    "LCD_COM2":[("U1","LCD_COM2"),("DS1","COM2")],
    "LCD_COM3":[("U1","LCD_COM3"),("DS1","COM3")],
    "LCD_SEG0":[("U1","LCD_SEG0"),("DS1","SEG0")],
    "LCD_SEG10":[("U1","LCD_SEG10"),("DS1","SEG10")],
    "CAPL":   [("U1","CAPL"),("C3","1")],
    "CAPH":   [("U1","CAPH"),("C3","2")],
    "NFC_SCK":[("U1","NFC_SCK"),("U2","SCLK")],
    "NFC_MOSI":[("U1","NFC_MOSI"),("U2","MOSI")],
    "NFC_MISO":[("U1","NFC_MISO"),("U2","MISO")],
    "NFC_CS": [("U1","NFC_CS"),("U2","CS")],
    "NFC_IRQ":[("U1","NFC_IRQ"),("U2","IRQ")],
    "ANT1":   [("U2","ANT1"),("C1","1")],
    "ANT2":   [("U2","ANT2"),("C2","1")],
    "ANT_A":  [("C1","2"),("L1","A")],
    "ANT_B":  [("C2","2"),("L1","B")],
    "SE_SDA": [("U1","SE_SDA"),("U3","SDA"),("R1","2")],
    "SE_SCL": [("U1","SE_SCL"),("U3","SCL"),("R2","2")],
    "BTN":    [("U1","BTN"),("SW1","1")],
    "SWDIO":  [("U1","SWDIO"),("J1","SWDIO")],
    "SWCLK":  [("U1","SWCLK"),("J1","SWCLK")],
    "RESET":  [("U1","RESET"),("J1","RESET")],
}

def pin_net(ref, pin):
    for net, members in NETS.items():
        if (ref, pin) in members:
            return net
    return None

def emit_symbol(ref, value, pins, x, y):
    """Emit a rectangular symbol with labeled pins and a global label per pin."""
    h = max(len(pins) * 2.54 + 5.08, 12.7)
    w = 25.4
    out = []
    lib = f"openT2OTP:{ref}"
    out.append(f'  (symbol (lib_id "{lib}") (at {x} {y} 0) (unit 1)')
    out.append(f'    (in_bom yes) (on_board yes) (uuid {U()})')
    out.append(f'    (property "Reference" "{ref}" (at {x} {y-h/2-2.54} 0)')
    out.append(f'      (effects (font (size 1.27 1.27))))')
    out.append(f'    (property "Value" "{value}" (at {x} {y+h/2+2.54} 0)')
    out.append(f'      (effects (font (size 1.27 1.27))))')
    out.append(f'  )')
    # global labels for each pin net, stacked to the right
    labels = []
    for i, p in enumerate(pins):
        net = pin_net(ref, p)
        if not net:
            continue
        ly = y - h/2 + 2.54 + i*2.54
        labels.append(
            f'  (global_label "{net}" (shape input) (at {x+w/2+5.08} {ly:.2f} 0)\n'
            f'    (effects (font (size 1.27 1.27)) (justify left))\n'
            f'    (uuid {U()}))'
        )
    return "\n".join(out), "\n".join(labels)

def main():
    print('(kicad_sch (version 20230121) (generator openT2OTP_gen)')
    print(f'  (uuid {U()})')
    print('  (paper "A3")')
    print('  (title_block')
    print('    (title "openT2OTP reference board")')
    print('    (company "openT2OTP")')
    print('    (comment 1 "Connectivity schematic - nets are the source of truth")')
    print('    (comment 2 "Replace generic symbols with real footprints, then route PCB")')
    print('  )')
    # embedded minimal lib_symbols so the file is self-contained
    print('  (lib_symbols')
    seen = set()
    for ref, value, pins in COMPONENTS:
        if ref in seen: continue
        seen.add(ref)
        h = max(len(pins) * 2.54 + 5.08, 12.7)
        print(f'    (symbol "openT2OTP:{ref}" (in_bom yes) (on_board yes)')
        print(f'      (property "Reference" "{ref}" (at 0 0 0) (effects (font (size 1.27 1.27))))')
        print(f'      (property "Value" "{value}" (at 0 0 0) (effects (font (size 1.27 1.27))))')
        print(f'      (symbol "{ref}_0_1"')
        print(f'        (rectangle (start -12.7 {h/2:.2f}) (end 12.7 {-h/2:.2f})')
        print(f'          (stroke (width 0.254) (type default)) (fill (type background))))')
        # pins
        for i, p in enumerate(pins):
            py = h/2 - 2.54 - i*2.54
            print(f'      (symbol "{ref}_1_1"')
            print(f'        (pin passive line (at -17.78 {py:.2f} 0) (length 5.08)')
            print(f'          (name "{p}" (effects (font (size 1.0 1.0))))')
            print(f'          (number "{i+1}" (effects (font (size 1.0 1.0))))))')
        print(f'    )')
    print('  )')
    # place instances in a grid
    positions = [
        (63.5, 100), (140, 60), (140, 130), (210, 60), (210, 120),
        (210, 160), (63.5, 180), (170, 180), (200, 180), (60, 40),
        (90, 40), (120, 40), (150, 40), (63.5, 220), (100, 220), (200, 220),
    ]
    for (ref, value, pins), (x, y) in zip(COMPONENTS, positions):
        sym, labels = emit_symbol(ref, value, pins, x, y)
        print(sym)
        if labels:
            print(labels)
    print(')')

if __name__ == "__main__":
    main()
