# System overview

## Problem

Households in typhoon and monsoon regions often lose connectivity first, while floodwater and energized ground-floor wiring create electrocution risk. BahayShield ULTRA demonstrates an **offline** controller that estimates rising risk from water level and pressure trend, then stages alerts and opens a safe demo load.

## Architecture

1. **Sense** - HC-SR04 water height, BME280 pressure, A0 pressure simulator for repeatable demos, RTC timebase.
2. **Decide** - Profile-weighted TRI with hysteresis thresholds; confirm window before latching power-cut.
3. **Act** - LED bar, buzzer, LCD pages, Relay 1 COM-NC on 5 V load, EEPROM event log.
4. **Operate** - Keypad, panel buttons, serial commands for demo and debug.

Diagrams: [`diagrams/`](diagrams/).

## Alert states

| State | Intent |
|-------|--------|
| NORMAL | Baseline monitoring |
| WATCH | Elevated TRI; gentle cues |
| WARNING1 | Higher urgency |
| POWER_CUT | Relay latched open after sustained critical conditions |
| EVACUATE | Highest urgency cues |

Exact numeric enter/exit thresholds are in `firmware/bahayshield-ultra/config.h`.

## Storm profiles

| Profile | Bias |
|---------|------|
| Habagat | Heavier water weight |
| Bagyo | Heavier pressure-slope weight |
| Flash Flood | Balanced water + slope |

## What this is not

- Not a certified life-safety product
- Not an AC transfer switch
- Not a cloud IoT dashboard
- Not a full-size Arduino Mega R3 as the final packaged controller (final MCU module is Mega 2560 Pro Mini)
