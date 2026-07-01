# Reference antenna coil geometry (keyfob dongle)

This is the starting geometry for the perimeter loop on the keyfob reference
board (the floorplan in `../HARDWARE.md`). Treat it as a starting point: measure
the fabricated coil's inductance and tune with `tune.py`.

## Outline

The keyfob body is roughly:

- overall board: ~55 mm (long axis) x ~28 mm (short axis)
- rounded left end + key-ring tab (outside the antenna area)
- LCD window at the right end, power button at the left

The antenna occupies the **perimeter**, just inside the board edge, wrapping
around the LCD and component cluster.

## Coil parameters (reference, pre-tuning)

| Parameter | Reference value | Notes |
| --- | --- | --- |
| Turns (N) | 4 | on top + bottom layer, via-stitched at corners |
| Track width | 0.30 mm | balance of resistance vs area |
| Track spacing | 0.30 mm | |
| Outer loop inset | 1.0 mm from board edge | keep-out from routing/case |
| Layers | 2 (top + bottom in series) | doubles effective turns for a thin board |
| Enclosed area | maximise | coupling scales with loop area |
| Estimated inductance | ~2-3 uH (measure!) | drives the tuning-cap value |

With ~2.5 uH measured, `tune.py` gives ~55 pF total (C1=C2 ~110 pF in series).

## Rules that must hold

1. **No copper, no battery, no metal inside the loop.** Ground pour or the coin
   cell inside the antenna area detunes and kills read range. Battery goes on the
   back, outside the loop footprint.
2. **Via-stitch the corners** where the loop transitions top<->bottom so the turns
   add in series.
3. **Keep the LCD zebra zone clear** of the coil.
4. **Symmetry** helps: route the two antenna terminals back to C1/C2 near the NFC
   front-end with matched trace lengths.

## Tuning

```
# after measuring the fabricated coil, e.g. 2.5 uH:
python3 tune.py -L 2.5u
```

Then fit the nearest C0G/NP0 E12 value and trim against a VNA resonance dip or by
maximising reader read-range. Re-check with the shell closed and battery fitted —
both shift resonance.

## Vendor references

Start from your NFC front-end vendor's antenna design guide and tuning
spreadsheet (ST and NXP both publish these for their 13.56 MHz front-ends) and
adapt the outline to this keyfob shape rather than computing the coil from
scratch.
