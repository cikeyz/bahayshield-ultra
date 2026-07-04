#include "event_log.h"
#include <Wire.h>

namespace {
constexpr uint8_t LOG_MAGIC0 = 'B';
constexpr uint8_t LOG_MAGIC1 = 'S';
constexpr uint8_t LOG_VERSION = 1;
constexpr uint8_t LOG_RECORD_SIZE = 20;
constexpr uint8_t LOG_PAGE_SIZE = 32;
constexpr uint16_t LOG_BYTES = 4096;
constexpr uint16_t LOG_RECORDS = LOG_BYTES / LOG_RECORD_SIZE;

uint8_t eepromAddress = 0;
uint16_t nextRecord = 0;
bool ready = false;

uint8_t checksum(const uint8_t *data, uint8_t length) {
  uint8_t value = 0;
  for (uint8_t i = 0; i < length; ++i) {
    value ^= data[i];
  }
  return value;
}

bool present() {
  Wire.beginTransmission(eepromAddress);
  return Wire.endTransmission() == 0;
}

void waitReady() {
  for (uint8_t attempt = 0; attempt < 50; ++attempt) {
    if (present()) {
      return;
    }
    delayMicroseconds(1000);
  }
}

bool readBytes(uint16_t address, uint8_t *data, uint8_t length) {
  Wire.beginTransmission(eepromAddress);
  Wire.write(uint8_t(address >> 8));
  Wire.write(uint8_t(address & 0xFF));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  uint8_t received = 0;
  Wire.requestFrom(eepromAddress, length);
  while (Wire.available() && received < length) {
    data[received++] = Wire.read();
  }
  return received == length;
}

bool writePageChunk(uint16_t address, const uint8_t *data, uint8_t length) {
  Wire.beginTransmission(eepromAddress);
  Wire.write(uint8_t(address >> 8));
  Wire.write(uint8_t(address & 0xFF));
  for (uint8_t i = 0; i < length; ++i) {
    Wire.write(data[i]);
  }
  if (Wire.endTransmission() != 0) {
    return false;
  }
  waitReady();
  return true;
}

bool writeBytesPaged(uint16_t address, const uint8_t *data, uint8_t length) {
  uint8_t offset = 0;
  while (offset < length) {
    const uint8_t pageRoom = LOG_PAGE_SIZE - (address % LOG_PAGE_SIZE);
    const uint8_t remaining = length - offset;
    const uint8_t chunk = remaining < pageRoom ? remaining : pageRoom;
    if (!writePageChunk(address, data + offset, chunk)) {
      return false;
    }
    address += chunk;
    offset += chunk;
  }
  return true;
}

bool recordValid(const uint8_t *record) {
  return record[0] == LOG_MAGIC0 && record[1] == LOG_MAGIC1 && record[2] == LOG_VERSION && record[19] == checksum(record, 19);
}

void put16(uint8_t *record, uint8_t index, int16_t value) {
  record[index] = uint8_t(uint16_t(value) >> 8);
  record[index + 1] = uint8_t(uint16_t(value) & 0xFF);
}

void put32(uint8_t *record, uint8_t index, uint32_t value) {
  record[index] = uint8_t(value >> 24);
  record[index + 1] = uint8_t(value >> 16);
  record[index + 2] = uint8_t(value >> 8);
  record[index + 3] = uint8_t(value);
}
}

bool eventLogBegin(uint8_t address) {
  eepromAddress = address;
  ready = present();
  nextRecord = 0;
  if (!ready) {
    return false;
  }

  uint8_t record[LOG_RECORD_SIZE];
  uint16_t found = 0;
  for (uint16_t i = 0; i < LOG_RECORDS; ++i) {
    if (!readBytes(i * LOG_RECORD_SIZE, record, LOG_RECORD_SIZE)) {
      ready = false;
      return false;
    }
    if (recordValid(record)) {
      found = i + 1;
    }
  }
  nextRecord = found >= LOG_RECORDS ? 0 : found;
  return true;
}

bool eventLogReady() {
  return ready;
}

uint16_t eventLogNextIndex() {
  return nextRecord;
}

bool eventLogWrite(uint8_t type, uint32_t secondsOfDay, uint8_t state, uint8_t profile, uint8_t tri, int16_t waterMm, int16_t pressureDeltaTenthHpa, int16_t tempTenthC, uint8_t humidityPct, uint8_t faults) {
  if (!ready) {
    return false;
  }

  uint8_t record[LOG_RECORD_SIZE] = {};
  record[0] = LOG_MAGIC0;
  record[1] = LOG_MAGIC1;
  record[2] = LOG_VERSION;
  record[3] = type;
  put32(record, 4, secondsOfDay);
  record[8] = state;
  record[9] = profile;
  record[10] = tri;
  put16(record, 11, waterMm);
  put16(record, 13, pressureDeltaTenthHpa);
  put16(record, 15, tempTenthC);
  record[17] = humidityPct;
  record[18] = faults;
  record[19] = checksum(record, 19);

  const uint16_t address = nextRecord * LOG_RECORD_SIZE;
  if (!writeBytesPaged(address, record, LOG_RECORD_SIZE)) {
    ready = false;
    return false;
  }

  ++nextRecord;
  if (nextRecord >= LOG_RECORDS) {
    nextRecord = 0;
  }
  return true;
}
