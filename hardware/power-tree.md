# Power tree

## Path

```text
USB-C PD source (or other protected DC source)
  -> input protection / fuse
  -> buck regulator (XL4015 5 A preferred; LM2596S accepted)
  -> regulated 5.00 V distribution
       -> logic branch: Mega Pro Mini 5V pin, I2C modules, LCD, LEDs, sensors
       -> actuator branch: relay coils, buzzer, demo load feed into Relay 1 COM
```

## Rules used on this project

1. Final board is the **Arduino Mega 2560 Pro Mini**. Feed regulated **5 V into the module `5V` pin**. Leave `VIN` unused.
2. Set and verify the buck output at **5.00 V** with a multimeter before connecting the Mega or peripherals.
3. Do not feed a selectable USB-C PD cable directly into the 5 V logic rail by default. Wrong selector voltage can destroy the board.
4. USB is for firmware upload and serial only. Avoid powering the logic rail from USB and external 5 V at the same time unless backfeed isolation is proven.
5. Relay contacts switch a **5 V demo load only**. No AC, no outlet, no household wiring, no 12 V demo-load branch in the minimum build.
6. Actuator current should not return through fragile sensor/I2C ground shortcuts; share ground intentionally.

## Measured bench notes (breadboard / G2 path)

On the external 5 V LED-strip demo path (Anker PD source through buck, bulk capacitor installed):

| Point | Approx. reading |
|-------|-----------------|
| External supply, no load | ~5.07 V |
| External supply, loaded / switching | ~5.01 V |
| Capacitor node, loaded | ~4.91 V |
| LED-strip current | ~0.39 A |

Breadboard and Dupont drop is expected. Final load wiring should use heavier power wire (project used 20 AWG class conductors for the load path).
