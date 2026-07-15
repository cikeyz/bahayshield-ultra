# Prototype-1 Release Gate

Do not export Gerbers until each item is signed off.

- Confirm whether this Prototype-1 board is being retired, revised, or used only as a routing reference. It is not the active release PCB until all blockers below are closed.
- Confirm Arduino Mega 2560 Pro header spacing, USB clearance, reset access, and orientation.
- Confirm J11 regulated 5 V input connector footprint and polarity.
- Confirm J10 demo-load connector footprint, load current, and isolation from any mains wiring.
- Confirm relay module input pin order and active level.
- Confirm ultrasonic, LCD, RTC, keypad, BMP280, buzzer, LED, pressure simulator, and button-ladder module pin order.
- Block release if the board does not expose six indicator outputs: five TRI bar LEDs plus one separate alert LED matching firmware pins D6 to D11.
- Block release if the BMP280 path still assumes `+3V3` without a locked module variant, documented regulator or level-shift design, and matching firmware and schematic notes.
- Block release if `+3V3`, J5 `BMP280_OPTION`, or C2 remains on the board only because of the old baseline rather than a documented final electrical decision.
- Block release if the connector pinout register does not match the production firmware pin map.
- Block release if the production firmware sketch has not compiled for Arduino Mega 2560 with the same pin map.
- Block release if the A1 button ladder resistor values and measured ADC windows are not recorded.
- Confirm enclosure standoffs against four M3 mounting holes.
- Run KiCad DRC after final footprint substitutions.
- Capture board screenshot, DRC report, BOM, and continuity test photos before defense.
