#include <Wire.h>
#include <Adafruit_BME280.h>

constexpr unsigned long PRINT_INTERVAL_MS = 1000;
constexpr float MIN_PLAUSIBLE_PRESSURE_HPA = 850.0F;
constexpr float MAX_PLAUSIBLE_PRESSURE_HPA = 1100.0F;
constexpr byte BME_RESET_REGISTER = 0xE0;
constexpr byte BME_RESET_COMMAND = 0xB6;
constexpr byte BME_STATUS_REGISTER = 0xF3;
constexpr byte BME_CTRL_HUM_REGISTER = 0xF2;
constexpr byte BME_CTRL_MEAS_REGISTER = 0xF4;
constexpr byte BME_FORCED_CTRL_HUM = 0x01;
constexpr byte BME_SLEEP_CTRL_MEAS = 0x24;
constexpr byte BME_FORCED_CTRL_MEAS = 0x25;
constexpr byte BME_STATUS_MEASURING = 0x08;
constexpr bool USE_FORCED_MODE = false;
constexpr float FALLBACK_COMPENSATION_TEMP_C = 23.0F;

Adafruit_BME280 bme;
unsigned long lastPrintMs = 0;
byte bmeAddress = 0;

struct BmeCalibration {
  uint16_t digT1;
  int16_t digT2;
  int16_t digT3;
  uint16_t digP1;
  int16_t digP2;
  int16_t digP3;
  int16_t digP4;
  int16_t digP5;
  int16_t digP6;
  int16_t digP7;
  int16_t digP8;
  int16_t digP9;
  uint8_t digH1;
  int16_t digH2;
  uint8_t digH3;
  int16_t digH4;
  int16_t digH5;
  int8_t digH6;
};

struct BmeRaw {
  int32_t pressure;
  int32_t temperature;
  int32_t humidity;
};

void softResetAddress(byte address) {
  Wire.beginTransmission(address);
  Wire.write(BME_RESET_REGISTER);
  Wire.write(BME_RESET_COMMAND);
  Wire.endTransmission();
}

void softResetPossibleBme() {
  softResetAddress(0x76);
  softResetAddress(0x77);
  delay(300);
}

bool beginBme(byte address) {
  if (bme.begin(address, &Wire)) {
    bmeAddress = address;
    if (USE_FORCED_MODE) {
      bme.setSampling(Adafruit_BME280::MODE_SLEEP,
                      Adafruit_BME280::SAMPLING_X1,
                      Adafruit_BME280::SAMPLING_X1,
                      Adafruit_BME280::SAMPLING_X1,
                      Adafruit_BME280::FILTER_OFF,
                      Adafruit_BME280::STANDBY_MS_1000);
    } else {
      bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                      Adafruit_BME280::SAMPLING_X1,
                      Adafruit_BME280::SAMPLING_X1,
                      Adafruit_BME280::SAMPLING_X1,
                      Adafruit_BME280::FILTER_OFF,
                      Adafruit_BME280::STANDBY_MS_1000);
    }
    return true;
  }
  return false;
}

byte readRegister(byte reg) {
  Wire.beginTransmission(bmeAddress);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)bmeAddress, 1);

  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

void writeRegister(byte reg, byte value) {
  Wire.beginTransmission(bmeAddress);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool waitMeasurementReady(unsigned long timeoutMs) {
  const unsigned long startMs = millis();

  while (millis() - startMs < timeoutMs) {
    if ((readRegister(BME_STATUS_REGISTER) & BME_STATUS_MEASURING) == 0) {
      return true;
    }
    delay(2);
  }

  return false;
}

bool triggerForcedMeasurement() {
  writeRegister(BME_CTRL_MEAS_REGISTER, BME_SLEEP_CTRL_MEAS);
  delay(2);
  writeRegister(BME_CTRL_HUM_REGISTER, BME_FORCED_CTRL_HUM);
  writeRegister(BME_CTRL_MEAS_REGISTER, BME_FORCED_CTRL_MEAS);
  delay(50);
  return waitMeasurementReady(250);
}

uint16_t read16(byte reg) {
  const uint16_t msb = readRegister(reg);
  const uint16_t lsb = readRegister(reg + 1);
  return (msb << 8) | lsb;
}

uint16_t read16Le(byte reg) {
  const uint16_t value = read16(reg);
  return (value >> 8) | (value << 8);
}

int16_t readS16Le(byte reg) {
  return (int16_t)read16Le(reg);
}

uint32_t read24(byte reg) {
  const uint32_t msb = readRegister(reg);
  const uint32_t lsb = readRegister(reg + 1);
  const uint32_t xlsb = readRegister(reg + 2);
  return (msb << 16) | (lsb << 8) | xlsb;
}

BmeCalibration readCalibration() {
  BmeCalibration cal;
  const byte h4Msb = readRegister(0xE4);
  const byte h45 = readRegister(0xE5);
  const byte h5Msb = readRegister(0xE6);

  cal.digT1 = read16Le(0x88);
  cal.digT2 = readS16Le(0x8A);
  cal.digT3 = readS16Le(0x8C);
  cal.digP1 = read16Le(0x8E);
  cal.digP2 = readS16Le(0x90);
  cal.digP3 = readS16Le(0x92);
  cal.digP4 = readS16Le(0x94);
  cal.digP5 = readS16Le(0x96);
  cal.digP6 = readS16Le(0x98);
  cal.digP7 = readS16Le(0x9A);
  cal.digP8 = readS16Le(0x9C);
  cal.digP9 = readS16Le(0x9E);
  cal.digH1 = readRegister(0xA1);
  cal.digH2 = readS16Le(0xE1);
  cal.digH3 = readRegister(0xE3);
  cal.digH4 = ((int16_t)((int8_t)h4Msb) << 4) | (h45 & 0x0F);
  cal.digH5 = ((int16_t)((int8_t)h5Msb) << 4) | (h45 >> 4);
  cal.digH6 = (int8_t)readRegister(0xE7);

  return cal;
}

BmeRaw readRaw() {
  BmeRaw raw;
  raw.pressure = (int32_t)(read24(0xF7) >> 4);
  raw.temperature = (int32_t)(read24(0xFA) >> 4);
  raw.humidity = (int32_t)read16(0xFD);
  return raw;
}

bool calibrationLooksSane(const BmeCalibration &cal) {
  return cal.digT1 != 0 && cal.digT1 != 0xFFFF && cal.digP1 != 0 && cal.digP1 != 0xFFFF;
}

float manualTemperatureC(const BmeCalibration &cal, int32_t adcT, int32_t &tFine) {
  int32_t var1 = ((((adcT >> 3) - ((int32_t)cal.digT1 << 1))) * ((int32_t)cal.digT2)) >> 11;
  int32_t var2 = (((((adcT >> 4) - ((int32_t)cal.digT1)) * ((adcT >> 4) - ((int32_t)cal.digT1))) >> 12) * ((int32_t)cal.digT3)) >> 14;
  tFine = var1 + var2;
  const int32_t temperature = (tFine * 5 + 128) >> 8;
  return temperature / 100.0F;
}

float manualPressureHpa(const BmeCalibration &cal, int32_t adcP, int32_t tFine) {
  int64_t var1 = ((int64_t)tFine) - 128000;
  int64_t var2 = var1 * var1 * (int64_t)cal.digP6;
  var2 = var2 + ((var1 * (int64_t)cal.digP5) << 17);
  var2 = var2 + (((int64_t)cal.digP4) << 35);
  var1 = ((var1 * var1 * (int64_t)cal.digP3) >> 8) + ((var1 * (int64_t)cal.digP2) << 12);
  var1 = (((((int64_t)1) << 47) + var1) * (int64_t)cal.digP1) >> 33;

  if (var1 == 0) {
    return NAN;
  }

  int64_t pressure = 1048576 - adcP;
  pressure = (((pressure << 31) - var2) * 3125) / var1;
  var1 = ((int64_t)cal.digP9 * (pressure >> 13) * (pressure >> 13)) >> 25;
  var2 = ((int64_t)cal.digP8 * pressure) >> 19;
  pressure = ((pressure + var1 + var2) >> 8) + (((int64_t)cal.digP7) << 4);
  return pressure / 25600.0F;
}

float manualHumidityPct(const BmeCalibration &cal, int32_t adcH, int32_t tFine) {
  int32_t value = tFine - 76800;
  value = (((((adcH << 14) - (((int32_t)cal.digH4) << 20) - (((int32_t)cal.digH5) * value)) + 16384) >> 15) *
           (((((((value * ((int32_t)cal.digH6)) >> 10) * (((value * ((int32_t)cal.digH3)) >> 11) + 32768)) >> 10) + 2097152) *
             ((int32_t)cal.digH2) + 8192) >> 14));
  value = value - (((((value >> 15) * (value >> 15)) >> 7) * ((int32_t)cal.digH1)) >> 4);

  if (value < 0) {
    value = 0;
  }
  if (value > 419430400) {
    value = 419430400;
  }
  return (value >> 12) / 1024.0F;
}

int32_t tFineFromTemperatureC(float temperatureC) {
  return (int32_t)((temperatureC * 100.0F * 256.0F) / 5.0F);
}

void printFloat1(float value) {
  Serial.print(value, 1);
}

void printHexByte(byte value) {
  if (value < 16) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

bool pressurePlausible(float pressureHpa) {
  return pressureHpa >= MIN_PLAUSIBLE_PRESSURE_HPA && pressureHpa <= MAX_PLAUSIBLE_PRESSURE_HPA;
}

void printRawDiagnostic() {
  bool sampleReady = true;
  if (USE_FORCED_MODE) {
    sampleReady = triggerForcedMeasurement();
  } else {
    delay(150);
  }

  const BmeCalibration cal = readCalibration();
  const BmeRaw raw = readRaw();
  int32_t tFine = 0;
  const float manualTempC = manualTemperatureC(cal, raw.temperature, tFine);
  const float manualPressureHpaValue = manualPressureHpa(cal, raw.pressure, tFine);
  const float manualHumidityPctValue = manualHumidityPct(cal, raw.humidity, tFine);
  const int32_t fallbackTFine = tFineFromTemperatureC(FALLBACK_COMPENSATION_TEMP_C);
  const float fallbackPressureHpa = manualPressureHpa(cal, raw.pressure, fallbackTFine);

  Serial.print(F("raw_id=0x"));
  printHexByte(readRegister(0xD0));
  Serial.print(F(" status=0x"));
  printHexByte(readRegister(0xF3));
  Serial.print(F(" ctrl_hum=0x"));
  printHexByte(readRegister(0xF2));
  Serial.print(F(" ctrl_meas=0x"));
  printHexByte(readRegister(0xF4));
  Serial.print(F(" config=0x"));
  printHexByte(readRegister(0xF5));
  Serial.print(F(" sample_mode="));
  Serial.print(USE_FORCED_MODE ? F("forced") : F("normal"));
  Serial.print(F(" ready="));
  Serial.print(sampleReady ? F("yes") : F("no"));
  Serial.println();

  Serial.print(F("cal_T1="));
  Serial.print(cal.digT1);
  Serial.print(F(" cal_T2="));
  Serial.print(cal.digT2);
  Serial.print(F(" cal_T3="));
  Serial.print(cal.digT3);
  Serial.print(F(" cal_P1="));
  Serial.print(cal.digP1);
  Serial.print(F(" cal_sane="));
  Serial.println(calibrationLooksSane(cal) ? F("yes") : F("no"));

  Serial.print(F("raw_adc_P="));
  Serial.print(raw.pressure);
  Serial.print(F(" raw_adc_T="));
  Serial.print(raw.temperature);
  Serial.print(F(" raw_adc_H="));
  Serial.println(raw.humidity);

  Serial.print(F("manual_pressure_hpa="));
  printFloat1(manualPressureHpaValue);
  Serial.print(F(" manual_pressure_ok="));
  Serial.print(pressurePlausible(manualPressureHpaValue) ? F("yes") : F("no"));
  Serial.print(F(" manual_temp_c="));
  printFloat1(manualTempC);
  Serial.print(F(" manual_humidity_pct="));
  printFloat1(manualHumidityPctValue);
  Serial.println();

  Serial.print(F("fallback_temp_c="));
  printFloat1(FALLBACK_COMPENSATION_TEMP_C);
  Serial.print(F(" fallback_pressure_hpa="));
  printFloat1(fallbackPressureHpa);
  Serial.print(F(" fallback_pressure_ok="));
  Serial.print(pressurePlausible(fallbackPressureHpa) ? F("yes") : F("no"));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println(F("BahayShield G2 M11 BME280"));
  Serial.println(F("connect only after voltage safety is accepted"));

  softResetPossibleBme();

  if (!beginBme(0x76) && !beginBme(0x77)) {
    Serial.println(F("bme280_not_found"));
    while (true) {
    }
  }

  Serial.print(F("bme280_addr=0x"));
  printHexByte(bmeAddress);
  Serial.println();
  Serial.print(F("sensor_id=0x"));
  printHexByte(bme.sensorID());
  Serial.println();

  printRawDiagnostic();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    bool sampleReady = true;
    if (USE_FORCED_MODE) {
      sampleReady = triggerForcedMeasurement();
    }
    const float pressureHpa = bme.readPressure() / 100.0F;
    const float temperatureC = bme.readTemperature();
    const float humidity = bme.readHumidity();
    const float altitudeM = bme.readAltitude(1013.25F);
    const BmeCalibration cal = readCalibration();
    const BmeRaw raw = readRaw();
    const float fallbackPressureHpa = manualPressureHpa(cal, raw.pressure, tFineFromTemperatureC(FALLBACK_COMPENSATION_TEMP_C));

    Serial.print(F("pressure_hpa="));
    printFloat1(pressureHpa);
    Serial.print(F(" pressure_ok="));
    Serial.print(pressurePlausible(pressureHpa) ? F("yes") : F("no"));
    Serial.print(F(" sample_mode="));
    Serial.print(USE_FORCED_MODE ? F("forced") : F("normal"));
    Serial.print(F(" ready="));
    Serial.print(sampleReady ? F("yes") : F("no"));
    Serial.print(F(" temp_c="));
    printFloat1(temperatureC);
    Serial.print(F(" humidity_pct="));
    printFloat1(humidity);
    Serial.print(F(" fallback_pressure_hpa="));
    printFloat1(fallbackPressureHpa);
    Serial.print(F(" fallback_pressure_ok="));
    Serial.print(pressurePlausible(fallbackPressureHpa) ? F("yes") : F("no"));
    Serial.print(F(" altitude_m_from_1013="));
    printFloat1(altitudeM);
    Serial.println();
  }
}
