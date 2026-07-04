#include <Wire.h>

constexpr unsigned long SCAN_INTERVAL_MS = 5000;

unsigned long lastScanMs = 0;

void printHex2(byte value) {
  if (value < 16) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void scanBus() {
  byte count = 0;

  Serial.println(F("i2c_scan_start"));

  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print(F("addr=0x"));
      printHex2(address);
      Serial.println();
      ++count;
    }
  }

  Serial.print(F("i2c_device_count="));
  Serial.println(count);
  Serial.println(F("i2c_scan_end"));
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println(F("BahayShield G2 M02 I2C scanner on Mega D20/D21"));
  scanBus();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = now;
    scanBus();
  }
}
