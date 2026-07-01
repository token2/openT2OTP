# KiCad — reference schematic & netlist

This directory holds a **connectivity schematic** for the openT2OTP reference
board, generated from the documented design. It is not a routed PCB — it gives
you the correct components and nets so you can drop in real footprints and lay
out the board.

## Files

| File | What it is |
| --- | --- |
| `openT2OTP.kicad_sch` | Self-contained KiCad 7 schematic (symbols embedded; opens with no external libraries). Every component is a labeled block; nets are expressed as global labels. |
| `openT2OTP.net` | KiCad-format netlist — the electrical source of truth (27 nets, 0 dangling). |
| `gen_schematic.py` | The schematic generator. Edit the `COMPONENTS` and `NETS` tables and regenerate. |
| `openT2OTP.kicad_pcb` | KiCad 7 PCB with the **board outline, mounting holes, and component placement** sized to fit the OT1 enclosure. **Placement only — not routed** (see below). |
| `gen_pcb.py` | The PCB generator. Board geometry (48×24 mm, 4× M2 holes at ±19.2/±7.3 mm) is derived from the OT1 STEP enclosure. |
| `gen_gerber.py` | Exports the mechanical Gerber + drill (outline + mounting holes). |
| `gerbers/` | Fabricable **board-outline** Gerber (`Edge_Cuts.gbr`) and Excellon drill (`PTH.drl`). Copper layers are absent by design — route first. |
| `placement.png` | **Suggested placement diagram** (see below) — an illustration, not a manufactured PCB. |
| `draw_placement.py` | Renders `placement.png` from the `.kicad_pcb`. |

## About `placement.png` — read this so it isn't misunderstood

`placement.png` is a **suggested component-placement diagram**. It shows where each
part would sit on a 48×24 mm board that fits the OT1 enclosure, with the arrangement
matched to the real reference board (battery clip at one end, IC cluster at the
other, LCD and button on the front).

**It is a drawing, not a photograph of a real board, and not a fabricable PCB.**
Specifically:

- The component shapes (QFNs, coin cell, LCD, crystal) are **illustrations** drawn
  to approximate size, not real vendor footprints.
- There is **no copper routing** — no traces, no ground plane, no vias. A real board
  needs all of that, done in KiCad with real footprints and DRC.
- A **Gerber/PCB never contains components anyway** — Gerbers describe only the bare
  board (copper, mask, silk, drill). The chips you see on a physical board are placed
  after manufacturing and never appear in Gerber files. So a "PCB image with chips on
  it" only comes from a **3D render of a fully-routed board**, which is a later step.

Use this diagram to understand and communicate the layout. To get to a board that
looks like the reference hardware: assign real footprints in `openT2OTP.kicad_pcb`,
route it, run DRC, then use KiCad's 3D viewer for a populated render and export the
full Gerber set for fab.

## The PCB: what's real and what isn't

The `.kicad_pcb` and the `gerbers/` are **mechanically real and honest about their
limits**:

- **Real & fabricable now:** the 48×24 mm rounded board outline and the four M2
  mounting holes at the enclosure's boss positions (±19.2, ±7.3 mm). A fab can cut
  and drill this exact blank and it will fit the OT1 case. The outline Gerber is a
  closed, dimension-checked profile.
- **Real placement:** all 15 components from the netlist are placed with courtyards,
  overlap-checked, front/back split (LCD on the front face; MCU, NFC, secure
  element, crystal, coin cell, passives, SWD on the back).
- **NOT done — you must do this in KiCad:** copper routing, the ground pour, via
  stitching, and especially the **13.56 MHz antenna matching**. The perimeter loop
  on `F.Cu` is placeholder geometry; measure the fabricated coil and retune with
  `../antenna/tune.py`. Run DRC, then export the full copper/mask/silk/paste Gerber
  set for production. **Do not send the current state to fab as a finished board.**

## Regenerate

```
python3 gen_schematic.py > openT2OTP.kicad_sch   # schematic + netlist
python3 gen_pcb.py       > openT2OTP.kicad_pcb   # outline + holes + placement
python3 gen_gerber.py                            # mechanical gerbers -> gerbers/
```

The net table in `gen_schematic.py` is validated to have no single-pin
(dangling) nets. See `../PINMAP.md` for the human-readable pin assignment.

## How to take this to a board

1. Open `openT2OTP.kicad_sch` in KiCad 7+.
2. Replace each generic block with the real library symbol + footprint for your
   chosen part (SAM L22 TQFP64, ST25R3916 QFN32, SE050, the LCD glass zebra
   footprint, 0402/0603 passives, CR2032 holder).
3. Keep the nets identical to `openT2OTP.net`.
4. Lay out the PCB per `../HARDWARE.md` (keyfob floorplan) and
   `../antenna/coil.md` (perimeter antenna + keep-out).
5. Design your own board outline and silkscreen — see `../CASING.md`.

## What this is not

- The `.kicad_pcb` is **placement, not a routed board** — no copper traces, no
  ground pour, no tuned antenna. Routing and DRC are yours to do in KiCad.
- Not tied to specific footprints — symbols and placement blocks are generic by
  design, so the netlist stays valid whatever exact parts you source. Swap in real
  vendor footprints before routing.
