# Pin map and I2C map

Final controller: **Arduino Mega 2560 Pro Mini** (ATmega2560, 5 V logic).

Arduino IDE board package: **Arduino Mega or Mega 2560**.

## Digital / analog

| Signal | Pin | Notes |
|--------|-----|--------|
| HC-SR04 TRIG | D2 | Output |
| HC-SR04 ECHO | D3 | Input |
| Relay 1 (IN1) | D4 | Active LOW |
| Relay 2 (IN2) | D5 | Active LOW; spare / optional |
| TRI LED G1 | D6 | Safe band |
| TRI LED G2 | D7 | |
| TRI LED Y | D8 | |
| TRI LED O | D9 | |
| TRI LED R | D10 | |
| Alert LED | D11 | |
| Buzzer | D12 | Active piezo |
| I2C SDA | D20 | Mega hardware I2C |
| I2C SCL | D21 | Mega hardware I2C |
| Pressure simulator | A0 | Potentiometer; demo storm curve |
| Button ladder | A1 | Yellow / Blue / Green |

Source of truth in firmware: `firmware/bahayshield-ultra/pins.h`.

## I2C addresses (as used in production config)

| Device | Address | Notes |
|--------|---------|--------|
| 20x4 LCD backpack | `0x27` primary, `0x3F` fallback | Direct `Wire` driver |
| PCF8574 keypad expander | `0x20` | Transposed key map handled in firmware |
| DS3231 RTC | `0x68` | |
| AT24C32 EEPROM | `0x57` | Event log |
| BME280 | `0x76` | Expected chip id `0x60` |

Source of truth: `firmware/bahayshield-ultra/config.h`.

## Relay module header order (delivered HL-52S class)

Logic header: `GND IN1 IN2 VCC`.

Contact group order per channel: `NO COM NC`.

- Relay 1: `COM` = load supply in, `NC` = load out (opens when coil energizes).
- `NO` unused on the minimum demo path.
