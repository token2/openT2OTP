# NFC antenna — design, geometry & tuning

The token is read and provisioned over NFC at **13.56 MHz** (ISO 14443). The
antenna is a **PCB copper loop** around the board perimeter — no antenna "part"
in the BOM, only etched copper plus two tuning capacitors (C1/C2).

## Files here

| File | What it is |
| --- | --- |
| `coil.md` | Reference coil geometry for the keyfob outline (turns, width, spacing, keep-out). |
| `tune.py` | Calculator: given the measured loop inductance, prints the matching capacitance for 13.56 MHz. |

## Quick tune

```
# after measuring the fabricated coil (e.g. 2.5 uH):
python3 tune.py -L 2.5u
```

Fit the nearest C0G/NP0 E12 value across C1/C2, then trim on the bench against a
VNA resonance dip or by maximising reader read-range. Re-check with the shell
closed and battery fitted — both shift resonance.

## Rules that must hold

1. **No copper, no battery, no metal inside the loop** — it detunes the coil and
   kills read range. Battery goes on the back, outside the loop footprint.
2. **Via-stitch the corners** so top/bottom turns add in series.
3. **Keep the LCD zebra zone clear** of the coil.
4. **Symmetric terminals** back to C1/C2 near the NFC front-end.

## Vendor references

Start from your NFC front-end vendor's antenna design guide and tuning
spreadsheet (ST and NXP publish these for their 13.56 MHz front-ends) and adapt
the outline to the keyfob shape rather than computing the coil from scratch.
