# Demo procedure (tabletop)

Prerequisites: regulated 5.00 V verified, production firmware uploaded to the **Mega 2560 Pro Mini**, demo load still disconnected for the first boot check.

## Cold start

1. Power the control unit from the buck-regulated 5 V rail.
2. Confirm LCD boots, serial status prints at 115200 baud, and relay LEDs are off (inactive).
3. Confirm I2C scan-era devices respond: LCD, PCF8574, DS3231, AT24C32, BME280 as available.
4. Connect the 5 V LED-strip load path only after the above looks healthy.

## Scenario A: rising water

1. Start dry under the HC-SR04 mast.
2. Add water toward the marked levels (15 / 30 / 55 mm class markers).
3. Observe TRI LEDs, LCD water reading, and buzzer cadence.
4. At sustained critical risk, confirm Relay 1 opens the COM-NC load path (strip off).

## Scenario B: pressure-led storm curve

1. Select Bagyo profile (`2` on serial or keypad path).
2. Sweep the A0 pressure potentiometer to simulate falling pressure.
3. Show slope / baseline contribution on LCD pages without claiming humidity drives cutoff.

## Recovery

1. Return water and pressure inputs to safe levels.
2. Use acknowledge (serial `a` / green button / keypad A) only when safe conditions hold.
3. Confirm load path restores on NC when the coil is released.

## Stop rules

- Stop if the 5 V rail collapses, a module heats abnormally, or the relay chatters unexpectedly.
- Never attach AC loads for the demo.
