# PCB

## Rev A (active)

Path: [`rev-a/`](rev-a/)

KiCad project for the BahayShield ULTRA motherboard that sockets an **Arduino Mega 2560 Pro Mini** on 100-mil headers, plus connectors for I2C modules, relay logic, load path, indicators, and power protection.

Open:

```text
pcb/rev-a/BahayShield-ULTRA-RevA-Manual.kicad_pro
```

Local symbol/footprint libraries are under `rev-a/libraries/`.

### Honest status

- Schematic captures the intended nets (MCU headers, I2C, relay inputs, load terminals, protection, bulk caps, TRI LEDs, buzzer drive).
- Treat fabrication as **re-validate before order**: re-run ERC/DRC on your KiCad version, check connector pin order against the **delivered** relay header (`GND IN1 IN2 VCC`) and contact group (`NO COM NC`), and confirm module keep-outs against the printed enclosure.
- This repo does not claim a certified production Gerber release package.

## Prototype-1 (archive)

Path: [`archive/prototype-1/`](archive/prototype-1/)

Early low-voltage routing baseline. Kept for history. It is not the final pin/feature set (indicator count and sensor options differ from the locked design). Prefer Rev A.
