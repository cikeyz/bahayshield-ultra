# Bring-up sketches

Single-purpose tests used while integrating the **Mega 2560 Pro** and modules. Upload one sketch at a time. Board target: **Arduino Mega or Mega 2560**.

| Sketch | Purpose |
|--------|---------|
| M00 | Upload + serial smoke |
| M02 | I2C scanner |
| M03 | 20x4 LCD direct |
| M04 | PCF8574 keypad |
| M05 | DS3231 + AT24C32 |
| M07 | A0 pressure pot |
| M08 | A1 button ladder |
| M09 | TRI LEDs + buzzer |
| M10 | HC-SR04 distance |
| M11 | BME280 read |
| M12 | Relay active-level |
| M13 | Relay COM-NC demo load |
| M14 | Full breadboard integration |
| M15 | Mast geometry calibration |
| M16 | Water-level marker response |

M11 and M14 need Adafruit BME280 (+ dependencies). Most others use Arduino core + `Wire` only.
