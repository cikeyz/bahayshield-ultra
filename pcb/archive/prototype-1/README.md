# BahayShield ULTRA Prototype-1

Prototype-1 is a low-voltage KiCad PCB baseline for the BahayShield ULTRA defense-room simulation unit. It is an interface and distribution board, not a mains-power controller.

Do not treat this as the current release PCB. It passed the original DRC check, but it no longer satisfies the active implementation requirements by itself.

## Contents

- `bahayshield_ultra_prototype_1.kicad_pro`: KiCad project file.
- `bahayshield_ultra_prototype_1.kicad_pcb`: PCB-authoritative board with footprints, pad nets, board outline, mounting holes, and routed low-voltage nets.
- `bahayshield_ultra_prototype_1.kicad_sch`: schematic stub that points reviewers to the board baseline and pinout register.
- `connector-pinout-register.csv`: connector pin map.
- `docs/design-baseline.md`: design intent and constraints.
- `docs/validation-report.md`: build and DRC summary.
- `manufacturing/release-gate.md`: checks required before fabrication.
- `reports/bom.csv`: grouped BOM.

## Baseline Status

- Date: 2026-05-12
- Components: 21
- Pins assigned: 70
- Named nets: 17
- References: J1A, J1B, J1C, J1D, J1E, J2, J3, J4, J5, J6, J7, J8, J9, J10, J11, J12, J13, R1, R2, C1, C2

## Current Release Blockers

- The accepted implementation requires five TRI bar LEDs plus one separate alert LED on D6 to D11. This baseline exposes only `LED_GREEN`, `LED_YELLOW`, and `LED_RED` through J13.
- The active procurement policy rejects unclear BMP280 voltage variants. This baseline still includes a `+3V3` path, J5 `BMP280_OPTION`, and C2 on `+3V3`; those are unresolved until the final BMP280 module and voltage policy are locked.
- The baseline is a PCB routing proof, not a firmware-validated system. No production BahayShield sketch is tied to this board yet.
- The baseline has no evidence package proving actual purchased connector orientation, module footprint fit, enclosure clearance, or relay-load wiring.

Do not fabricate until the release-gate file is cleared against actual bought modules, connector orientation, enclosure clearances, relay-load wiring, six-indicator output requirements, BMP280 voltage policy, and firmware pin-map proof.
