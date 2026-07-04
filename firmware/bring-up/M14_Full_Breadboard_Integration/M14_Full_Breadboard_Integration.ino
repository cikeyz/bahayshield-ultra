#include <Wire.h>
#include <Adafruit_BME280.h>

constexpr byte PIN_US_TRIG = 2;
constexpr byte PIN_US_ECHO = 3;
constexpr byte PIN_RELAY1 = 4;
constexpr byte PIN_RELAY2 = 5;
constexpr byte PIN_LED_G1 = 6;
constexpr byte PIN_LED_G2 = 7;
constexpr byte PIN_LED_Y = 8;
constexpr byte PIN_LED_O = 9;
constexpr byte PIN_LED_R = 10;
constexpr byte PIN_LED_ALERT = 11;
constexpr byte PIN_BUZZER = 12;
constexpr byte PIN_PRESSURE = A0;
constexpr byte PIN_BUTTONS = A1;

constexpr byte RELAY_ACTIVE = LOW;
constexpr byte RELAY_INACTIVE = HIGH;

constexpr byte LCD_ADDR_PRIMARY = 0x27;
constexpr byte LCD_ADDR_FALLBACK = 0x3F;
constexpr byte PCF8574_ADDR = 0x20;
constexpr byte DS3231_ADDR = 0x68;
constexpr byte AT24C32_ADDR = 0x57;

constexpr byte LCD_BACKLIGHT = 0x08;
constexpr byte LCD_ENABLE = 0x04;
constexpr byte LCD_RS = 0x01;

constexpr unsigned long STATUS_INTERVAL_MS = 1000;
constexpr unsigned long LCD_INTERVAL_MS = 500;
constexpr unsigned long BUTTON_INTERVAL_MS = 40;
constexpr unsigned long KEYPAD_INTERVAL_MS = 35;
constexpr unsigned long BME_INTERVAL_MS = 2000;
constexpr unsigned long RTC_INTERVAL_MS = 1000;
constexpr unsigned long ULTRASONIC_INTERVAL_MS = 250;
constexpr unsigned long ULTRASONIC_TIMEOUT_US = 30000;
constexpr unsigned long BUZZER_MS = 120;

const byte LED_PINS[] = {PIN_LED_G1, PIN_LED_G2, PIN_LED_Y, PIN_LED_O, PIN_LED_R};

const char KEYMAP[4][4] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};

enum ButtonId : byte {
  BTN_NONE,
  BTN_YELLOW_CUT,
  BTN_BLUE_VIEW,
  BTN_GREEN_ACK,
  BTN_AMBIGUOUS
};

enum EchoState : byte {
  ECHO_IDLE,
  ECHO_WAIT_RISE,
  ECHO_WAIT_FALL
};

Adafruit_BME280 bme;

byte lcdAddress = 0;
byte lcdPage = 0;
byte bmeAddress = 0;
bool lcdOk = false;
bool keypadOk = false;
bool rtcOk = false;
bool eepromOk = false;
bool bmeOk = false;
bool relayCut = false;
bool ultrasonicValid = false;

ButtonId stableButton = BTN_NONE;
ButtonId pendingButton = BTN_NONE;
char lastKey = 0;

EchoState echoState = ECHO_IDLE;

unsigned long lastStatusMs = 0;
unsigned long lastLcdMs = 0;
unsigned long lastButtonMs = 0;
unsigned long lastKeypadMs = 0;
unsigned long lastBmeMs = 0;
unsigned long lastRtcMs = 0;
unsigned long lastUltrasonicMs = 0;
unsigned long buttonPendingSinceMs = 0;
unsigned long buzzerUntilMs = 0;
unsigned long triggerStartUs = 0;
unsigned long echoRiseUs = 0;
unsigned long ultrasonicEchoUs = 0;

int pressureTenthHpa = 0;
int waterMm = 0;
int ultrasonicDistanceMm = -1;
int bmePressureTenthHpa = 0;
int bmeTempTenthC = 0;
int bmeHumidityTenthPct = 0;
byte rtcHours = 0;
byte rtcMinutes = 0;
byte rtcSeconds = 0;

bool i2cPresent(byte address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void printHexByte(byte value) {
  if (value < 16) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

byte bcdToDec(byte value) {
  return ((value >> 4) * 10) + (value & 0x0F);
}

void printTenth(int value) {
  Serial.print(value / 10);
  Serial.print('.');
  Serial.print(abs(value % 10));
}

void lcdWriteExpander(byte value) {
  if (!lcdOk) {
    return;
  }
  Wire.beginTransmission(lcdAddress);
  Wire.write(value | LCD_BACKLIGHT);
  if (Wire.endTransmission() != 0) {
    lcdOk = false;
  }
}

void lcdPulse(byte value) {
  lcdWriteExpander(value | LCD_ENABLE);
  delayMicroseconds(1);
  lcdWriteExpander(value & ~LCD_ENABLE);
  delayMicroseconds(50);
}

void lcdWrite4(byte value, byte mode) {
  lcdPulse((value & 0xF0) | mode);
}

void lcdWrite(byte value, byte mode) {
  lcdWrite4(value & 0xF0, mode);
  lcdWrite4((value << 4) & 0xF0, mode);
}

void lcdCommand(byte value) {
  lcdWrite(value, 0);
  if (value == 0x01 || value == 0x02) {
    delay(2);
  }
}

void lcdData(byte value) {
  lcdWrite(value, LCD_RS);
}

void lcdSetCursor(byte col, byte row) {
  static const byte offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcdCommand(0x80 | (col + offsets[row]));
}

void lcdPrintFixed(const char *text) {
  byte count = 0;
  while (*text && count < 20) {
    lcdData(*text++);
    ++count;
  }
  while (count < 20) {
    lcdData(' ');
    ++count;
  }
}

void lcdInit() {
  delay(50);
  lcdWrite4(0x30, 0);
  delay(5);
  lcdWrite4(0x30, 0);
  delayMicroseconds(150);
  lcdWrite4(0x30, 0);
  lcdWrite4(0x20, 0);
  lcdCommand(0x28);
  lcdCommand(0x08);
  lcdCommand(0x01);
  lcdCommand(0x06);
  lcdCommand(0x0C);
}

void pcfWrite(byte value) {
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(value);
  if (Wire.endTransmission() != 0) {
    keypadOk = false;
  }
}

byte pcfRead() {
  Wire.requestFrom(PCF8574_ADDR, byte(1));
  if (Wire.available()) {
    return Wire.read();
  }
  keypadOk = false;
  return 0xFF;
}

char scanKeypad() {
  if (!keypadOk) {
    return 0;
  }
  for (byte row = 0; row < 4; ++row) {
    byte output = 0xFF;
    output &= ~(1 << row);
    pcfWrite(output);
    delayMicroseconds(80);
    const byte input = pcfRead();
    for (byte col = 0; col < 4; ++col) {
      const byte bitIndex = col + 4;
      if ((input & (1 << bitIndex)) == 0) {
        pcfWrite(0xFF);
        return KEYMAP[row][col];
      }
    }
  }
  pcfWrite(0xFF);
  return 0;
}

ButtonId decodeButton(int adc) {
  if (adc >= 900 && adc <= 1000) {
    return BTN_YELLOW_CUT;
  }
  if (adc >= 780 && adc <= 890) {
    return BTN_BLUE_VIEW;
  }
  if (adc >= 630 && adc <= 760) {
    return BTN_GREEN_ACK;
  }
  if (adc >= 0 && adc <= 80) {
    return BTN_NONE;
  }
  return BTN_AMBIGUOUS;
}

const __FlashStringHelper *buttonName(ButtonId button) {
  switch (button) {
    case BTN_YELLOW_CUT:
      return F("YELLOW_CUT");
    case BTN_BLUE_VIEW:
      return F("BLUE_VIEW");
    case BTN_GREEN_ACK:
      return F("GREEN_ACK");
    case BTN_AMBIGUOUS:
      return F("AMBIGUOUS");
    default:
      return F("NONE");
  }
}

void setRelayCut(bool cut) {
  relayCut = cut;
  digitalWrite(PIN_RELAY1, relayCut ? RELAY_ACTIVE : RELAY_INACTIVE);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE);
}

void startBeep(unsigned long now) {
  digitalWrite(PIN_BUZZER, HIGH);
  buzzerUntilMs = now + BUZZER_MS;
}

void updateBuzzer(unsigned long now) {
  if (buzzerUntilMs != 0 && now - buzzerUntilMs < 0x80000000UL) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerUntilMs = 0;
  }
}

void updatePressure() {
  const int adc = analogRead(PIN_PRESSURE);
  pressureTenthHpa = map(adc, 0, 1023, 9850, 10130);
}

void updateIndicators() {
  byte level = 0;
  if (ultrasonicValid) {
    if (waterMm >= 55) {
      level = 5;
    } else if (waterMm >= 45) {
      level = 4;
    } else if (waterMm >= 30) {
      level = 3;
    } else if (waterMm >= 15) {
      level = 2;
    } else if (waterMm > 0) {
      level = 1;
    }
  }

  for (byte i = 0; i < sizeof(LED_PINS); ++i) {
    digitalWrite(LED_PINS[i], i < level ? HIGH : LOW);
  }
  digitalWrite(PIN_LED_ALERT, (ultrasonicValid && waterMm >= 55) || relayCut ? HIGH : LOW);
}

void triggerUltrasonic() {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  triggerStartUs = micros();
  echoState = ECHO_WAIT_RISE;
}

void updateUltrasonic(unsigned long nowMs) {
  const unsigned long nowUs = micros();

  if (echoState == ECHO_IDLE && nowMs - lastUltrasonicMs >= ULTRASONIC_INTERVAL_MS) {
    lastUltrasonicMs = nowMs;
    triggerUltrasonic();
    return;
  }

  if (echoState == ECHO_WAIT_RISE) {
    if (digitalRead(PIN_US_ECHO) == HIGH) {
      echoRiseUs = nowUs;
      echoState = ECHO_WAIT_FALL;
    } else if (nowUs - triggerStartUs >= ULTRASONIC_TIMEOUT_US) {
      ultrasonicValid = false;
      ultrasonicEchoUs = 0;
      ultrasonicDistanceMm = -1;
      echoState = ECHO_IDLE;
    }
    return;
  }

  if (echoState == ECHO_WAIT_FALL) {
    if (digitalRead(PIN_US_ECHO) == LOW) {
      const unsigned long echoUs = nowUs - echoRiseUs;
      const int distanceMm = int((echoUs * 10UL) / 58UL);
      ultrasonicEchoUs = echoUs;
      ultrasonicDistanceMm = distanceMm;
      int computedWaterMm = 120 - distanceMm;
      if (computedWaterMm < 0) {
        computedWaterMm = 0;
      }
      if (computedWaterMm > 70) {
        computedWaterMm = 70;
      }
      waterMm = computedWaterMm;
      ultrasonicValid = true;
      echoState = ECHO_IDLE;
    } else if (nowUs - triggerStartUs >= ULTRASONIC_TIMEOUT_US) {
      ultrasonicValid = false;
      ultrasonicEchoUs = 0;
      ultrasonicDistanceMm = -1;
      echoState = ECHO_IDLE;
    }
  }
}

bool readRtcTime() {
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(byte(0x00));
  if (Wire.endTransmission() != 0) {
    return false;
  }
  Wire.requestFrom(DS3231_ADDR, byte(3));
  if (Wire.available() < 3) {
    return false;
  }
  rtcSeconds = bcdToDec(Wire.read() & 0x7F);
  rtcMinutes = bcdToDec(Wire.read());
  rtcHours = bcdToDec(Wire.read() & 0x3F);
  return true;
}

void updateRtc(unsigned long now) {
  if (now - lastRtcMs < RTC_INTERVAL_MS) {
    return;
  }
  lastRtcMs = now;
  rtcOk = i2cPresent(DS3231_ADDR) && readRtcTime();
  eepromOk = i2cPresent(AT24C32_ADDR);
}

void updateBme(unsigned long now) {
  if (!bmeOk || now - lastBmeMs < BME_INTERVAL_MS) {
    return;
  }
  lastBmeMs = now;
  bmePressureTenthHpa = int((bme.readPressure() / 10.0F) + 0.5F);
  bmeTempTenthC = int((bme.readTemperature() * 10.0F) + 0.5F);
  bmeHumidityTenthPct = int((bme.readHumidity() * 10.0F) + 0.5F);
}

void cycleLcdPage() {
  ++lcdPage;
  if (lcdPage >= 3) {
    lcdPage = 0;
  }
}

void handleAction(const __FlashStringHelper *source, char action, unsigned long now) {
  Serial.print(source);
  Serial.print(F("_action="));
  Serial.println(action);

  switch (action) {
    case 'n':
      setRelayCut(false);
      startBeep(now);
      break;
    case 'c':
      setRelayCut(true);
      startBeep(now);
      break;
    case 'v':
      cycleLcdPage();
      break;
    case 'b':
      startBeep(now);
      break;
    case 'e':
      break;
  }
}

void handleButton(ButtonId button, unsigned long now) {
  if (button == BTN_YELLOW_CUT) {
    handleAction(F("button"), 'c', now);
  } else if (button == BTN_BLUE_VIEW) {
    handleAction(F("button"), 'v', now);
  } else if (button == BTN_GREEN_ACK) {
    handleAction(F("button"), 'n', now);
  }
}

void updateButtons(unsigned long now) {
  if (now - lastButtonMs < BUTTON_INTERVAL_MS) {
    return;
  }
  lastButtonMs = now;

  const ButtonId rawButton = decodeButton(analogRead(PIN_BUTTONS));
  if (rawButton != pendingButton) {
    pendingButton = rawButton;
    buttonPendingSinceMs = now;
  }

  if (now - buttonPendingSinceMs >= 80 && stableButton != pendingButton) {
    stableButton = pendingButton;
    Serial.print(F("button="));
    Serial.println(buttonName(stableButton));
    if (stableButton != BTN_NONE && stableButton != BTN_AMBIGUOUS) {
      handleButton(stableButton, now);
    }
  }
}

void updateKeypad(unsigned long now) {
  if (now - lastKeypadMs < KEYPAD_INTERVAL_MS) {
    return;
  }
  lastKeypadMs = now;

  const char key = scanKeypad();
  if (key != 0 && key != lastKey) {
    Serial.print(F("key="));
    Serial.println(key);
    if (key == 'A') {
      handleAction(F("keypad"), 'n', now);
    } else if (key == 'B') {
      handleAction(F("keypad"), 'c', now);
    } else if (key == 'C') {
      handleAction(F("keypad"), 'b', now);
    } else if (key == 'D') {
      handleAction(F("keypad"), 'v', now);
    } else if (key == '#') {
      handleAction(F("keypad"), 'e', now);
    }
  }
  lastKey = key;
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

bool writeEepromSmokeRecord() {
  byte record[8] = {'M', '1', '4', byte(relayCut), byte(waterMm), byte(pressureTenthHpa / 10), rtcMinutes, 0};
  record[7] = checksum(record, 7);

  Wire.beginTransmission(AT24C32_ADDR);
  Wire.write(byte(0x00));
  Wire.write(byte(0x10));
  for (byte i = 0; i < sizeof(record); ++i) {
    Wire.write(record[i]);
  }
  if (Wire.endTransmission() != 0) {
    return false;
  }
  waitEepromReady();

  Wire.beginTransmission(AT24C32_ADDR);
  Wire.write(byte(0x00));
  Wire.write(byte(0x10));
  if (Wire.endTransmission() != 0) {
    return false;
  }
  Wire.requestFrom(AT24C32_ADDR, byte(sizeof(record)));
  if (Wire.available() < sizeof(record)) {
    return false;
  }

  byte readback[8];
  for (byte i = 0; i < sizeof(readback); ++i) {
    readback[i] = Wire.read();
  }
  return readback[0] == 'M' && readback[1] == '1' && readback[2] == '4' && readback[7] == checksum(readback, 7);
}

void handleSerial(unsigned long now) {
  while (Serial.available()) {
    const char command = Serial.read();
    if (command == 'n') {
      handleAction(F("serial"), 'n', now);
    } else if (command == 'c') {
      handleAction(F("serial"), 'c', now);
    } else if (command == 'b') {
      handleAction(F("serial"), 'b', now);
    } else if (command == 'v') {
      handleAction(F("serial"), 'v', now);
    } else if (command == 'e') {
      Serial.print(F("eeprom_smoke="));
      Serial.println(eepromOk && writeEepromSmokeRecord() ? F("ok") : F("fail"));
    } else if (command == '?') {
      Serial.println(F("commands: n normal, c cut, b beep, v view, e eeprom, ? menu"));
      Serial.println(F("keypad: A normal, B cut, C beep, D view, # eeprom"));
      Serial.println(F("buttons: yellow cut, blue view, green normal"));
    }
  }
}

void drawLcd(unsigned long now) {
  if (!lcdOk || now - lastLcdMs < LCD_INTERVAL_MS) {
    return;
  }
  lastLcdMs = now;

  char line[21];

  if (lcdPage == 0) {
    lcdSetCursor(0, 0);
    lcdPrintFixed("BahayShield M14");
    snprintf(line, sizeof(line), "Water:%3dmm %s", waterMm, ultrasonicValid ? "OK" : "NO");
    lcdSetCursor(0, 1);
    lcdPrintFixed(line);
    snprintf(line, sizeof(line), "Psim:%4d.%1dhPa", pressureTenthHpa / 10, abs(pressureTenthHpa % 10));
    lcdSetCursor(0, 2);
    lcdPrintFixed(line);
    snprintf(line, sizeof(line), "Relay:%s", relayCut ? "CUT" : "NORMAL");
    lcdSetCursor(0, 3);
    lcdPrintFixed(line);
    return;
  }

  if (lcdPage == 1) {
    lcdSetCursor(0, 0);
    lcdPrintFixed("I2C status");
    snprintf(line, sizeof(line), "LCD:%s KEY:%s", lcdOk ? "OK" : "NO", keypadOk ? "OK" : "NO");
    lcdSetCursor(0, 1);
    lcdPrintFixed(line);
    snprintf(line, sizeof(line), "RTC:%s EEP:%s", rtcOk ? "OK" : "NO", eepromOk ? "OK" : "NO");
    lcdSetCursor(0, 2);
    lcdPrintFixed(line);
    snprintf(line, sizeof(line), "BME:%s 0x%02X", bmeOk ? "OK" : "NO", bmeAddress);
    lcdSetCursor(0, 3);
    lcdPrintFixed(line);
    return;
  }

  lcdSetCursor(0, 0);
  lcdPrintFixed("Env context only");
  snprintf(line, sizeof(line), "BME P:%4d.%1d", bmePressureTenthHpa / 10, abs(bmePressureTenthHpa % 10));
  lcdSetCursor(0, 1);
  lcdPrintFixed(line);
  snprintf(line, sizeof(line), "T:%3d.%1d H:%3d.%1d", bmeTempTenthC / 10, abs(bmeTempTenthC % 10), bmeHumidityTenthPct / 10, abs(bmeHumidityTenthPct % 10));
  lcdSetCursor(0, 2);
  lcdPrintFixed(line);
  snprintf(line, sizeof(line), "%02u:%02u:%02u", rtcHours, rtcMinutes, rtcSeconds);
  lcdSetCursor(0, 3);
  lcdPrintFixed(line);
}

void printStatus(unsigned long now) {
  if (now - lastStatusMs < STATUS_INTERVAL_MS) {
    return;
  }
  lastStatusMs = now;

  Serial.print(F("m14 t_ms="));
  Serial.print(now);
  Serial.print(F(" water_mm="));
  Serial.print(ultrasonicValid ? waterMm : -1);
  Serial.print(F(" distance_mm="));
  Serial.print(ultrasonicValid ? ultrasonicDistanceMm : -1);
  Serial.print(F(" echo_us="));
  Serial.print(ultrasonicValid ? ultrasonicEchoUs : 0);
  Serial.print(F(" pressure_sim_hpa="));
  printTenth(pressureTenthHpa);
  Serial.print(F(" bme_ok="));
  Serial.print(bmeOk ? F("yes") : F("no"));
  Serial.print(F(" bme_hpa="));
  printTenth(bmePressureTenthHpa);
  Serial.print(F(" temp_c="));
  printTenth(bmeTempTenthC);
  Serial.print(F(" humidity_pct="));
  printTenth(bmeHumidityTenthPct);
  Serial.print(F(" rtc="));
  Serial.print(rtcOk ? F("ok") : F("missing"));
  Serial.print(F(" relay="));
  Serial.print(relayCut ? F("cut") : F("normal"));
  Serial.print(F(" button="));
  Serial.print(buttonName(stableButton));
  Serial.print(F(" lcd="));
  Serial.print(lcdOk ? F("ok") : F("missing"));
  Serial.print(F(" keypad="));
  Serial.println(keypadOk ? F("ok") : F("missing"));
}

void setupI2cDevices() {
  if (i2cPresent(LCD_ADDR_PRIMARY)) {
    lcdAddress = LCD_ADDR_PRIMARY;
    lcdOk = true;
  } else if (i2cPresent(LCD_ADDR_FALLBACK)) {
    lcdAddress = LCD_ADDR_FALLBACK;
    lcdOk = true;
  }

  if (lcdOk) {
    lcdInit();
  }

  keypadOk = i2cPresent(PCF8574_ADDR);
  if (keypadOk) {
    pcfWrite(0xFF);
  }

  rtcOk = i2cPresent(DS3231_ADDR) && readRtcTime();
  eepromOk = i2cPresent(AT24C32_ADDR);

  if (bme.begin(0x76, &Wire)) {
    bmeAddress = 0x76;
    bmeOk = true;
  } else if (bme.begin(0x77, &Wire)) {
    bmeAddress = 0x77;
    bmeOk = true;
  }

  if (bmeOk) {
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::FILTER_OFF,
                    Adafruit_BME280::STANDBY_MS_1000);
  }
}

void printStartup() {
  Serial.println(F("BahayShield G2 M14 full breadboard integration"));
  Serial.println(F("not production firmware"));
  Serial.print(F("lcd="));
  Serial.print(lcdOk ? F("ok addr=0x") : F("missing addr=0x"));
  printHexByte(lcdAddress);
  Serial.println();
  Serial.print(F("keypad_pcf8574_0x20="));
  Serial.println(keypadOk ? F("ok") : F("missing"));
  Serial.print(F("ds3231_0x68="));
  Serial.println(rtcOk ? F("ok") : F("missing"));
  Serial.print(F("at24c32_0x57="));
  Serial.println(eepromOk ? F("ok") : F("missing"));
  Serial.print(F("bme280="));
  Serial.print(bmeOk ? F("ok addr=0x") : F("missing addr=0x"));
  printHexByte(bmeAddress);
  Serial.println();
  Serial.println(F("commands: n normal, c cut, b beep, v view, e eeprom, ? menu"));
}

void setup() {
  digitalWrite(PIN_RELAY1, RELAY_INACTIVE);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE);

  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_LED_G1, OUTPUT);
  pinMode(PIN_LED_G2, OUTPUT);
  pinMode(PIN_LED_Y, OUTPUT);
  pinMode(PIN_LED_O, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_US_TRIG, LOW);
  setRelayCut(false);
  digitalWrite(PIN_BUZZER, LOW);

  Serial.begin(115200);
  Wire.begin();
  setupI2cDevices();
  updatePressure();
  updateBme(0);
  printStartup();
}

void loop() {
  const unsigned long now = millis();

  updatePressure();
  updateUltrasonic(now);
  updateButtons(now);
  updateKeypad(now);
  updateBme(now);
  updateRtc(now);
  updateIndicators();
  updateBuzzer(now);
  handleSerial(now);
  drawLcd(now);
  printStatus(now);
}
