# Production firmware

Board: **Arduino Mega 2560 Pro** (ATmega2560, 5 V).

Arduino IDE: **Arduino Mega or Mega 2560**.

## Folder

```text
firmware/bahayshield-ultra/
├── bahayshield-ultra.ino
├── config.h
├── pins.h
├── event_log.h
└── event_log.cpp
```

## Libraries

```text
Wire
Adafruit BME280
Adafruit Unified Sensor
Adafruit BusIO
```

## Serial

Baud: `115200`.

| Command | Action |
|---------|--------|
| `c` | Manual relay cut |
| `a` | Acknowledge / reset when safe |
| `v` | Cycle LCD page |
| `m` | Mute buzzer |
| `1` / `2` / `3` | Habagat / Bagyo / Flash Flood |
| `e` | Write one event-log smoke record |
| `?` | Help |

Upload with the external load disconnected. Confirm startup prints, LCD, I2C, inactive relay, and 5.00 V rails before connecting the LED-strip load path.
