# PCB

## Rev A

Path: [`rev-a/`](rev-a/)

KiCad motherboard for BahayShield ULTRA. It sockets an **Arduino Mega 2560 Pro** on 100-mil headers and brings out I2C modules, relay logic, load path, indicators, and power protection.

```text
pcb/rev-a/BahayShield-ULTRA-RevA-Manual.kicad_pro
```

Local symbol and footprint libraries: `rev-a/libraries/`.

### Status

- Schematic covers the intended nets: MCU headers, I2C, relay inputs, load terminals, protection, bulk caps, TRI LEDs, buzzer drive.
- Before ordering boards, re-run ERC/DRC on your KiCad version, confirm relay header order (`GND IN1 IN2 VCC`) and contact group (`NO COM NC`) against the module you hold, and check keep-outs against the printed enclosure.
- This tree is a design package, not a certified production Gerber drop.

## Prototype-1 (archive)

Path: [`archive/prototype-1/`](archive/prototype-1/)

Early low-voltage routing baseline. Kept for history. Prefer Rev A for current pin and feature set.
