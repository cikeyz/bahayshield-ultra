#include <Wire.h>

constexpr byte PCF8574_ADDR_PRIMARY = 0x20;
constexpr unsigned long SCAN_INTERVAL_MS = 30;
constexpr unsigned long PRINT_INTERVAL_MS = 1000;

const char KEYMAP[4][4] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};

byte pcfAddress = PCF8574_ADDR_PRIMARY;
char lastKey = 0;
unsigned long lastScanMs = 0;
unsigned long lastPrintMs = 0;

bool i2cPresent(byte address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void pcfWrite(byte value) {
  Wire.beginTransmission(pcfAddress);
  Wire.write(value);
  Wire.endTransmission();
}

byte pcfRead() {
  Wire.requestFrom(pcfAddress, byte(1));
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF;
}

char scanKeypad() {
  for (byte row = 0; row < 4; ++row) {
    byte output = 0xFF;
    output &= ~(1 << row);
    pcfWrite(output);
    delayMicroseconds(80);

    const byte input = pcfRead();

    for (byte col = 0; col < 4; ++col) {
      const byte bitIndex = col + 4;
      if ((input & (1 << bitIndex)) == 0) {
        return KEYMAP[row][col];
      }
    }
  }

  pcfWrite(0xFF);
  return 0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!i2cPresent(pcfAddress)) {
    Serial.println(F("pcf8574_missing_at_0x20"));
    Serial.println(F("scan bus with M02 and update pcfAddress"));
    while (true) {
    }
  }

  pcfWrite(0xFF);
  Serial.println(F("BahayShield G2 M04 PCF8574 keypad"));
  Serial.println(F("press keys: 1 2 3 A / 4 5 6 B / 7 8 9 C / * 0 # D"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = now;
    const char key = scanKeypad();

    if (key != 0 && key != lastKey) {
      Serial.print(F("key="));
      Serial.println(key);
    }

    lastKey = key;
  }

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    Serial.println(F("waiting_for_key"));
  }
}
