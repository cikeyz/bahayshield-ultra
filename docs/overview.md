# System overview

## Problem

During typhoons and monsoon floods, homes often lose connectivity first. Water and energized ground-floor wiring raise electrocution risk. BahayShield ULTRA is an offline controller that estimates rising risk from water level and pressure trend, stages alerts, and opens a safe low-voltage demo load.

## Architecture

1. **Sense** - HC-SR04 water height, BME280 pressure, A0 pressure simulator for repeatable demos, RTC timebase.
2. **Decide** - Profile-weighted TRI with hysteresis; confirm window before latching power-cut.
3. **Act** - LED bar, buzzer, LCD pages, Relay 1 COM-NC on 5 V load, EEPROM event log.
4. **Operate** - Keypad, panel buttons, serial commands.

Diagrams: [`diagrams/`](diagrams/).

## Alert states

| State | Intent |
|-------|--------|
| NORMAL | Baseline monitoring |
| WATCH | Elevated TRI |
| WARNING1 | Higher urgency |
| POWER_CUT | Relay latched open after sustained critical conditions |
| EVACUATE | Highest urgency cues |

Threshold constants live in `firmware/bahayshield-ultra/config.h`.

## Storm profiles

| Profile | Bias |
|---------|------|
| Habagat | Heavier water weight |
| Bagyo | Heavier pressure-slope weight |
| Flash Flood | Balanced water + slope |

## Controller

Final packaged controller is the compact **Arduino Mega 2560 Pro** (ATmega2560). Full-size Mega 2560 boards are fine for early breadboard work; pin numbers and the IDE board package stay the same.
