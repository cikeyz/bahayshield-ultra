# Enclosure and mast

Printed parts for the control unit and HC-SR04 mast. Designed around the **Mega 2560 Pro Mini** motherboard stack, internal relay/buck bays, and a top-panel BME280 vent cage open to room air.

## STL set

| File | Role |
|------|------|
| `stl/bahayshield-base-tray.stl` | Base tray |
| `stl/bahayshield-top-panel.stl` | Top panel |
| `stl/bme280-vent-cage.stl` | Room-air sensor cage |
| `stl/hcsr04-faceplate.stl` | Sensor faceplate |
| `stl/hcsr04-mast-base.stl` | Mast base |
| `stl/hcsr04-mast-upright-132mm.stl` | Upright |
| `stl/hcsr04-mast-arm.stl` | Arm |

Editable source: `scad/bahayshield-enclosure.scad` and `scad/generate_enclosure_stl.py`.

## Print notes

| Setting | Suggestion |
|---------|------------|
| Material | PETG preferred for functional housing; PLA+ OK for fit checks |
| Layer height | 0.20 mm final |
| Walls | 4+ |
| Infill | ~25% |
| Order | Faceplate and BME280 cage first for fit, then top panel and base, then mast |

Expect holes to need light reaming after print. Keep BME280 exposed to room air; do not seal it inside a closed volume.
