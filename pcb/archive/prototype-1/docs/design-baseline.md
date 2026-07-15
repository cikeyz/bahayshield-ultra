# Prototype-1 Design Baseline

## Intent

This PCB is a low-voltage interface board for the Arduino Mega 2560 Pro based BahayShield ULTRA simulation unit. It breaks out Mega-side control nets to field connectors for the ultrasonic water-level sensor, I2C LCD/RTC/keypad chain, optional BMP280 module, button ladder, pressure simulator, relay inputs, buzzer, and status LEDs.

This file describes the Prototype-1 routing baseline, not the current release design. The active implementation requires six indicator outputs and a locked BMP280 voltage policy before fabrication.

## Electrical Boundaries

- Input power is regulated 5 V only through J11.
- No mains voltage is allowed on this PCB.
- J10 is reserved for the low-voltage demo load path only.
- I2C nets use 4.7 k pullups to 5 V through R1 and R2.
- C1 provides local 5 V decoupling. C2 is tied to the old `+3V3` BMP280 option and must be removed, revised, or formally justified before any release board.
- Power and relay/buzzer traces use wider track widths than logic traces.
- The final board must expose five TRI LED outputs plus one separate alert LED. The three-net `STATUS_LEDS` header in this baseline is incomplete.
- Do not add a default 3.3 V rail only to rescue an unknown BMP280 module. Lock a 5 V I2C-safe module or document a formal level-shifted redesign.

## Connector Policy

Connector pin order is frozen in `connector-pinout-register.csv` for Prototype-1. Physical connector series, wire exit, keying, and shrouding remain release-gate items because the real purchased modules can reverse pin order.

## Layout Policy

- Board size: 135 mm x 95 mm.
- Mounting holes: four M3-compatible holes near corners.
- Copper layers: routed on F.Cu in this baseline.
- Silk labels identify low-voltage status and revision.
- The board file is authoritative for Prototype-1 because the MCP schematic writer corrupts large repeated schematic insertions on this KiCad 10 Windows setup.
