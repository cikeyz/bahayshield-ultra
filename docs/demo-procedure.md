# Demo procedure

Prerequisites: regulated 5.00 V verified, production firmware on the **Mega 2560 Pro**, demo load disconnected for first boot check.

## Cold start

1. Power the control unit from the buck-regulated 5 V rail.
2. Confirm LCD boots, serial status at 115200 baud, and relay inactive.
3. Confirm I2C devices as available: LCD, PCF8574, DS3231, AT24C32, BME280.
4. Connect the 5 V LED-strip load path only after the above looks healthy.

## Scenario A: rising water

1. Start dry under the HC-SR04 mast.
2. Raise water toward the marked levels (15 / 30 / 55 mm class).
3. Watch TRI LEDs, LCD water reading, and buzzer cadence.
4. At sustained critical risk, confirm Relay 1 opens the COM-NC path (strip off).

## Scenario B: pressure-led storm curve

1. Select Bagyo profile (`2` on serial or keypad).
2. Sweep the A0 potentiometer to simulate falling pressure.
3. Show slope and baseline contribution on the LCD.

## Recovery

1. Return water and pressure inputs to safe levels.
2. Acknowledge (serial `a` / green button / keypad A) when safe conditions hold.
3. Confirm the load path restores when the coil releases.

## Stop rules

- Stop if the 5 V rail collapses, a module overheats, or the relay chatters.
- Never attach AC loads for the demo.
