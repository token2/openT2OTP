# Gerbers — board outline + drill only

These files define the **mechanical** board only:

- `openT2OTP-Edge_Cuts.gbr` — the 48 x 24 mm rounded board outline (RS-274X).
- `openT2OTP-PTH.drl` — Excellon drill: 4 x M2 (2.2 mm) mounting holes at the
  OT1 enclosure boss positions (+/-19.2, +/-7.3 mm).

They let a fab cut and drill the correctly shaped blank that fits the OT1 case.
**Copper layers are intentionally absent** — the board is not yet routed. Route
it in KiCad from `../openT2OTP.kicad_pcb`, run DRC, then export the full Gerber
set (copper, soldermask, silk, paste) for production.
