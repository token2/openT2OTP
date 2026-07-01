# Casing & industrial design

The enclosure and board outline in this reference are **deliberately our own**.
This repository open-sources how a TOTP token *works* — the firmware, the wire
protocol handling, the electrical design — not any particular product's *look and
feel*. The industrial design (shell shape, button placement, silkscreen, colours,
proportions) is intentionally distinct from other vendors' tokens to avoid trading
on anyone's trade dress or design IP.

If you fork this, keep that separation in mind: reuse the electronics and firmware
freely under their licenses, but design your own housing rather than copying an
existing product's appearance.

## Reference form factor: keyfob dongle

Our main reference is a **keyfob dongle** with:
- a segment LCD window at one end,
- a single power/show button at the other,
- a key-ring tab on one side,
- a two-part snap or ultrasonically-welded shell.

Chosen because it's the most buildable: a rigid board with vertical room for a
replaceable coin-cell holder, a tactile button, and a generous perimeter antenna.
See [`HARDWARE.md`](HARDWARE.md) §0 for the dongle-vs-card trade-offs.

**Distinct design cues** (make your own choices — these are just ours):
- an asymmetric outline rather than a plain rounded rectangle,
- button and LCD arranged along the long axis,
- our own silkscreen pattern and wordmark placement.

## Card variation

A card variant is documented as a variation (same schematic, thinner stack-up,
printed antenna, sealed cell). If you build a card, again: use your own card
artwork and outline, not a copy of an existing product's card face.

## Mechanical checklist

- LCD window aligned to the glass active area; light pipe or plain window (no
  backlight needed for a transflective display).
- Button plunger reaches the PCB switch with the shell closed.
- Coin-cell access: a hatch for the keyfob (replaceable) or a sealed pocket for
  the card.
- Antenna keep-out respected inside the shell — no metal foils or coatings over
  the coil.
- Housing holds the LCD zebra strip under compression against the PCB pads.
