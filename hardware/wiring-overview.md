# Wiring overview

## MCU form factor

Final harness and PCB assume the **Arduino Mega 2560 Pro Mini** module seated on 100-mil headers. Early tests may use a full-size Mega 2560 form factor; pin numbers in software stay the same because both are ATmega2560 Mega-class boards.

## I2C bus

Shared bus on D20/D21:

- LCD
- PCF8574 keypad
- DS3231 + AT24C32
- BME280 (off-board, vented to room air; not sealed inside a closed box and not inside the wet diorama)

4.7 kΩ pull-ups are on the design intent for the custom board I2C net.

## Relay load path (minimum demo)

```text
External 5 V load supply +  --->  Relay1 COM
Relay1 NC                   --->  LED strip +
LED strip -                 --->  External 5 V load supply -

Relay logic VCC/GND/IN1 from Mega-side 5 V / GND / D4
```

- Active level: **LOW** energizes coil (verified on delivered module).
- Safe idle: inputs pulled inactive so reset/boot does not cut the load spuriously.
- Never put mains on the contact terminals.

## Ultrasonic mast geometry (locked demo)

- Sensor face height: 120 mm above inside tray floor (design target)
- Water markers: 0 / 15 / 30 / 45 / 55 mm from tray floor
- Keep a clear measurement well under the sensor; aim vertical

Firmware water mapping constants live in `firmware/bahayshield-ultra/config.h`.
