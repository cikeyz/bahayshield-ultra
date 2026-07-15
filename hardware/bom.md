# Bill of materials

One simulation unit. No prices or vendor order IDs.

## Electronics

| Item | Spec notes |
|------|------------|
| Arduino Mega 2560 Pro | ATmega2560, 5 V, CH340-class USB, 100-mil headers |
| HC-SR04 | Ultrasonic distance |
| BME280 breakout | I2C; pressure required; 5 V-safe module preferred |
| DS3231 + AT24C32 module | RTC + EEPROM |
| 20x4 I2C LCD | Backpack `0x27` common |
| PCF8574 + 4x4 keypad | I2C expander at `0x20` in this build |
| 2-channel 5 V relay module | HL-52S / Songle SRD-05VDC-SL-C class |
| Active piezo buzzer | Enclosure-mounted |
| LEDs + resistors | 5 TRI + 1 alert |
| 3 tactile buttons + ladder resistors | On A1 |
| Potentiometer | A0 pressure simulator |
| Buck regulator | XL4015 5 A preferred |
| Bulk electrolytic | 470 uF class on 5 V rail |
| Fuse holder + fuse | Input protection |
| Schottky reverse protection | e.g. 1N5822 class on input path |
| Headers / JST-XH / KF301 terminals | Interconnects |
| Hookup wire | Signal + ~20 AWG for load path |

## Mechanical / demo

| Item | Spec notes |
|------|------------|
| 3D-printed enclosure set | Base tray, top panel, BME280 cage |
| HC-SR04 mast set | Base, upright, arm, faceplate |
| House diorama + clear water tray | Markers at 0 / 15 / 30 / 45 / 55 mm |
| 5 V LED strip | Demo load on Relay 1 COM-NC |

## Out of minimum scope

- Whole-unit battery backup
- 12 V demo-load branch
- Real AC contactors or household wiring
- Outdoor weatherproof deployment
