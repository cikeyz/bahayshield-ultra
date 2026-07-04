#include <Wire.h>

constexpr byte DS3231_ADDR = 0x68;
constexpr byte AT24C32_ADDR = 0x57;
constexpr unsigned long PRINT_INTERVAL_MS = 1000;

unsigned long lastPrintMs = 0;

byte bcdToDec(byte value) {
  return ((value >> 4) * 10) + (value & 0x0F);
}

bool i2cPresent(byte address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

byte readRegister(byte device, byte reg) {
  Wire.beginTransmission(device);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(device, byte(1));
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

void print2(byte value) {
  if (value < 10) {
    Serial.print('0');
  }
  Serial.print(value);
}

void printTime() {
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(byte(0x00));
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDR, byte(3));

  if (Wire.available() < 3) {
    Serial.println(F("rtc_read_failed"));
    return;
  }

  const byte seconds = bcdToDec(Wire.read() & 0x7F);
  const byte minutes = bcdToDec(Wire.read());
  const byte hours = bcdToDec(Wire.read() & 0x3F);

  Serial.print(F("rtc_time="));
  print2(hours);
  Serial.print(':');
  print2(minutes);
  Serial.print(':');
  print2(seconds);
  Serial.println();
}

void waitEepromReady() {
  for (byte attempt = 0; attempt < 50; ++attempt) {
    Wire.beginTransmission(AT24C32_ADDR);
    if (Wire.endTransmission() == 0) {
      return;
    }
    delay(1);
  }
}

byte checksum(const byte *data, byte length) {
  byte value = 0;
  for (byte i = 0; i < length; ++i) {
    value ^= data[i];
  }
  return value;
}

bool writeRecord() {
  byte record[8] = {'B', 'S', 1, 5, 0, 0, 0, 0};
  record[7] = checksum(record, 7);

  Wire.beginTransmission(AT24C32_ADDR);
  Wire.write(byte(0x00));
  Wire.write(byte(0x00));
  for (byte i = 0; i < sizeof(record); ++i) {
    Wire.write(record[i]);
  }
  if (Wire.endTransmission() != 0) {
    return false;
  }

  waitEepromReady();
  return true;
}

bool readRecord() {
  byte record[8];

  Wire.beginTransmission(AT24C32_ADDR);
  Wire.write(byte(0x00));
  Wire.write(byte(0x00));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  Wire.requestFrom(AT24C32_ADDR, byte(sizeof(record)));
  if (Wire.available() < sizeof(record)) {
    return false;
  }

  for (byte i = 0; i < sizeof(record); ++i) {
    record[i] = Wire.read();
  }

  const bool ok = record[0] == 'B' && record[1] == 'S' && record[7] == checksum(record, 7);
  Serial.print(F("eeprom_record_ok="));
  Serial.println(ok ? F("yes") : F("no"));
  return ok;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println(F("BahayShield G2 M05 DS3231 AT24C32"));
  Serial.println(F("keep CR2032 removed until charge path is accepted"));

  Serial.print(F("ds3231_present="));
  Serial.println(i2cPresent(DS3231_ADDR) ? F("yes") : F("no"));
  Serial.print(F("at24c32_present="));
  Serial.println(i2cPresent(AT24C32_ADDR) ? F("yes") : F("no"));

  if (i2cPresent(AT24C32_ADDR)) {
    Serial.print(F("eeprom_write="));
    Serial.println(writeRecord() ? F("ok") : F("fail"));
    readRecord();
  }
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    if (i2cPresent(DS3231_ADDR)) {
      printTime();
    }
  }
}
