#include <Wire.h>
#include <Adafruit_BME280.h>
#include "pins.h"
#include "config.h"
#include "event_log.h"

enum class ButtonId : uint8_t {
  None,
  YellowCut,
  BlueView,
  GreenAck,
  Ambiguous
};

enum class EchoState : uint8_t {
  Idle,
  WaitRise,
  WaitFall
};

enum class AlertState : uint8_t {
  Normal,
  Watch,
  Warning1,
  Warning2PowerCut,
  Evacuate
};

enum class Profile : uint8_t {
  Habagat,
  Bagyo,
  FlashFlood
};

struct ProfileConfig {
  const char *name;
  uint8_t waterWeight;
  uint8_t slopeWeight;
  uint8_t baselineWeight;
};

constexpr ProfileConfig PROFILES[] = {
  {"HABAGAT", 55, 20, 25},
  {"BAGYO", 25, 50, 25},
  {"FLASH", 45, 35, 20}
};

constexpr uint8_t LED_PINS[] = {PIN_LED_G1, PIN_LED_G2, PIN_LED_Y, PIN_LED_O, PIN_LED_R};
constexpr char KEYMAP[4][4] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};

constexpr uint8_t LCD_BACKLIGHT = 0x08;
constexpr uint8_t LCD_ENABLE = 0x04;
constexpr uint8_t LCD_RS = 0x01;

constexpr uint8_t FAULT_BME = 0x01;
constexpr uint8_t FAULT_RTC = 0x02;
constexpr uint8_t FAULT_LOG = 0x04;
constexpr uint8_t FAULT_LCD = 0x08;
constexpr uint8_t FAULT_KEYPAD = 0x10;
constexpr uint8_t FAULT_ULTRASONIC = 0x20;
constexpr uint8_t FAULT_BUTTON = 0x40;

Adafruit_BME280 bme;

Profile profile = Profile::Bagyo;
AlertState alertState = AlertState::Normal;
AlertState previousAlertState = AlertState::Normal;
EchoState echoState = EchoState::Idle;
ButtonId stableButton = ButtonId::None;
ButtonId pendingButton = ButtonId::None;

uint8_t lcdAddress = 0;
uint8_t lcdPage = 0;
uint8_t tri = 0;
uint8_t waterScore = 0;
uint8_t slopeScore = 0;
uint8_t baselineScore = 0;
uint8_t faults = 0;
uint8_t previousFaults = 0;
uint8_t rtcHours = 0;
uint8_t rtcMinutes = 0;
uint8_t rtcSeconds = 0;

bool lcdOk = false;
bool keypadOk = false;
bool rtcOk = false;
bool logOk = false;
bool bmeOk = false;
bool relayCutLatched = false;
bool manualCutLatched = false;
bool buzzerMuted = false;
bool ultrasonicValid = false;
bool warning2Condition = false;
bool safeCondition = false;

char lastKey = 0;

uint32_t lastButtonMs = 0;
uint32_t lastKeypadMs = 0;
uint32_t lastPressureMs = 0;
uint32_t lastBmeMs = 0;
uint32_t lastRtcMs = 0;
uint32_t lastTriMs = 0;
uint32_t lastLcdMs = 0;
uint32_t lastSerialMs = 0;
uint32_t lastUltrasonicMs = 0;
uint32_t buttonPendingSinceMs = 0;
uint32_t triggerStartUs = 0;
uint32_t echoRiseUs = 0;
uint32_t ultrasonicEchoUs = 0;
uint32_t lastUltrasonicOkMs = 0;
uint32_t warning2SinceMs = 0;
uint32_t safeSinceMs = 0;
uint32_t actionBeepStartedMs = 0;
uint32_t lastPressureSampleMs = 0;

int16_t pressureTenthHpa = PRESSURE_BASELINE_TENTH_HPA;
int16_t previousPressureTenthHpa = PRESSURE_BASELINE_TENTH_HPA;
int16_t pressureSlopeTenthHpaPerMin = 0;
int16_t baselineTenthHpa = PRESSURE_BASELINE_TENTH_HPA;
int16_t bmePressureTenthHpa = 0;
int16_t bmeTempTenthC = 0;
uint8_t bmeHumidityPct = 0;
int16_t waterMm = 0;
int16_t ultrasonicDistanceMm = -1;

uint8_t clampScore(int16_t value) {
  if (value <= 0) {
    return 0;
  }
  if (value >= 100) {
    return 100;
  }
  return uint8_t(value);
}

bool i2cPresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint8_t bcdToDec(uint8_t value) {
  return ((value >> 4) * 10) + (value & 0x0F);
}

uint32_t secondsOfDay() {
  return uint32_t(rtcHours) * 3600UL + uint32_t(rtcMinutes) * 60UL + rtcSeconds;
}

int16_t readPressureTenthHpa() {
  uint32_t total = 0;
  for (uint8_t i = 0; i < PRESSURE_ADC_SAMPLES; ++i) {
    total += analogRead(PIN_PRESSURE);
    delayMicroseconds(250);
  }
  const uint16_t adc = uint16_t((total + (PRESSURE_ADC_SAMPLES / 2)) / PRESSURE_ADC_SAMPLES);
  return map(adc, 0, 1023, PRESSURE_MIN_TENTH_HPA, PRESSURE_MAX_TENTH_HPA);
}

uint16_t readButtonsAdc() {
  uint32_t total = 0;
  for (uint8_t i = 0; i < BUTTON_ADC_SAMPLES; ++i) {
    total += analogRead(PIN_BUTTONS);
    delayMicroseconds(250);
  }
  return uint16_t((total + (BUTTON_ADC_SAMPLES / 2)) / BUTTON_ADC_SAMPLES);
}

void printHexByte(uint8_t value) {
  if (value < 16) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void printTenth(int16_t value) {
  if (value < 0) {
    Serial.print('-');
    value = -value;
  }
  Serial.print(value / 10);
  Serial.print('.');
  Serial.print(value % 10);
}

const char *stateName(AlertState state) {
  switch (state) {
    case AlertState::Watch:
      return "WATCH";
    case AlertState::Warning1:
      return "WARNING1";
    case AlertState::Warning2PowerCut:
      return "POWER_CUT";
    case AlertState::Evacuate:
      return "EVACUATE";
    default:
      return "NORMAL";
  }
}

const char *buttonName(ButtonId button) {
  switch (button) {
    case ButtonId::YellowCut:
      return "YELLOW_CUT";
    case ButtonId::BlueView:
      return "BLUE_VIEW";
    case ButtonId::GreenAck:
      return "GREEN_ACK";
    case ButtonId::Ambiguous:
      return "AMBIGUOUS";
    default:
      return "NONE";
  }
}

const ProfileConfig &currentProfile() {
  return PROFILES[uint8_t(profile)];
}

void logEvent(uint8_t type) {
  logOk = eventLogWrite(type, secondsOfDay(), uint8_t(alertState), uint8_t(profile), tri, waterMm, pressureTenthHpa - baselineTenthHpa, bmeTempTenthC, bmeHumidityPct, faults);
  if (!logOk) {
    faults |= FAULT_LOG;
  }
}

void setRelayCut(bool cut) {
  relayCutLatched = cut;
  digitalWrite(PIN_RELAY1, relayCutLatched ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE_LEVEL);
}

void startActionBeep(uint32_t now) {
  actionBeepStartedMs = now;
  digitalWrite(PIN_BUZZER, HIGH);
}

void lcdWriteExpander(uint8_t value) {
  if (!lcdOk) {
    return;
  }
  Wire.beginTransmission(lcdAddress);
  Wire.write(value | LCD_BACKLIGHT);
  if (Wire.endTransmission() != 0) {
    lcdOk = false;
    faults |= FAULT_LCD;
  }
}

void lcdPulse(uint8_t value) {
  lcdWriteExpander(value | LCD_ENABLE);
  delayMicroseconds(1);
  lcdWriteExpander(value & ~LCD_ENABLE);
  delayMicroseconds(50);
}

void lcdWrite4(uint8_t value, uint8_t mode) {
  lcdPulse((value & 0xF0) | mode);
}

void lcdWrite(uint8_t value, uint8_t mode) {
  lcdWrite4(value & 0xF0, mode);
  lcdWrite4((value << 4) & 0xF0, mode);
}

void lcdCommand(uint8_t value) {
  lcdWrite(value, 0);
  if (value == 0x01 || value == 0x02) {
    delayMicroseconds(2000);
  }
}

void lcdData(uint8_t value) {
  lcdWrite(value, LCD_RS);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  static const uint8_t offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcdCommand(0x80 | (col + offsets[row]));
}

void lcdPrintFixed(const char *text) {
  uint8_t count = 0;
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
  for (uint8_t i = 0; i < 50; ++i) {
    delayMicroseconds(1000);
  }
  lcdWrite4(0x30, 0);
  delayMicroseconds(5000);
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

void pcfWrite(uint8_t value) {
  if (!keypadOk) {
    return;
  }
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(value);
  if (Wire.endTransmission() != 0) {
    keypadOk = false;
    faults |= FAULT_KEYPAD;
  }
}

uint8_t pcfRead() {
  Wire.requestFrom(PCF8574_ADDR, uint8_t(1));
  if (Wire.available()) {
    return Wire.read();
  }
  keypadOk = false;
  faults |= FAULT_KEYPAD;
  return 0xFF;
}

char scanKeypad() {
  if (!keypadOk) {
    return 0;
  }
  for (uint8_t row = 0; row < 4; ++row) {
    uint8_t output = 0xFF;
    output &= ~(1 << row);
    pcfWrite(output);
    delayMicroseconds(80);
    const uint8_t input = pcfRead();
    for (uint8_t col = 0; col < 4; ++col) {
      const uint8_t bitIndex = col + 4;
      if ((input & (1 << bitIndex)) == 0) {
        pcfWrite(0xFF);
        return KEYMAP[row][col];
      }
    }
  }
  pcfWrite(0xFF);
  return 0;
}

ButtonId decodeButton(uint16_t adc) {
  if (adc >= BUTTON_YELLOW_MIN && adc <= BUTTON_YELLOW_MAX) {
    return ButtonId::YellowCut;
  }
  if (adc >= BUTTON_BLUE_MIN && adc <= BUTTON_BLUE_MAX) {
    return ButtonId::BlueView;
  }
  if (adc >= BUTTON_GREEN_MIN && adc <= BUTTON_GREEN_MAX) {
    return ButtonId::GreenAck;
  }
  if (adc >= BUTTON_NONE_MIN && adc <= BUTTON_NONE_MAX) {
    return ButtonId::None;
  }
  return ButtonId::Ambiguous;
}

bool readRtcTime() {
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(uint8_t(0x00));
  if (Wire.endTransmission() != 0) {
    return false;
  }
  Wire.requestFrom(DS3231_ADDR, uint8_t(3));
  if (Wire.available() < 3) {
    return false;
  }
  rtcSeconds = bcdToDec(Wire.read() & 0x7F);
  rtcMinutes = bcdToDec(Wire.read());
  rtcHours = bcdToDec(Wire.read() & 0x3F);
  return true;
}

void updateRtc(uint32_t now) {
  if (now - lastRtcMs < RTC_INTERVAL_MS) {
    return;
  }
  lastRtcMs = now;
  rtcOk = i2cPresent(DS3231_ADDR) && readRtcTime();
  if (rtcOk) {
    faults &= ~FAULT_RTC;
  } else {
    faults |= FAULT_RTC;
  }
}

void updateBme(uint32_t now) {
  if (!bmeOk || now - lastBmeMs < BME_INTERVAL_MS) {
    return;
  }
  lastBmeMs = now;
  const float pressureHpa = bme.readPressure() / 100.0F;
  const float tempC = bme.readTemperature();
  const float humidity = bme.readHumidity();
  if (pressureHpa < 800.0F || pressureHpa > 1100.0F || tempC < -10.0F || tempC > 70.0F || humidity < 0.0F || humidity > 100.0F) {
    bmeOk = false;
    faults |= FAULT_BME;
    logEvent(EVENT_FAULT);
    return;
  }
  bmePressureTenthHpa = int16_t(pressureHpa * 10.0F + 0.5F);
  bmeTempTenthC = int16_t(tempC * 10.0F + (tempC >= 0.0F ? 0.5F : -0.5F));
  bmeHumidityPct = uint8_t(humidity + 0.5F);
  faults &= ~FAULT_BME;
}

void updatePressure(uint32_t now) {
  if (now - lastPressureMs < PRESSURE_INTERVAL_MS) {
    return;
  }
  lastPressureMs = now;
  previousPressureTenthHpa = pressureTenthHpa;
  const int16_t rawPressureTenthHpa = readPressureTenthHpa();
  int16_t deltaStep = rawPressureTenthHpa - pressureTenthHpa;
  if (deltaStep > PRESSURE_MAX_STEP_TENTH_HPA) {
    deltaStep = PRESSURE_MAX_STEP_TENTH_HPA;
  } else if (deltaStep < -PRESSURE_MAX_STEP_TENTH_HPA) {
    deltaStep = -PRESSURE_MAX_STEP_TENTH_HPA;
  }
  pressureTenthHpa += deltaStep;

  if (lastPressureSampleMs != 0) {
    const uint32_t elapsed = now - lastPressureSampleMs;
    if (elapsed > 0) {
      const int32_t delta = int32_t(pressureTenthHpa) - previousPressureTenthHpa;
      pressureSlopeTenthHpaPerMin = int16_t((delta * 60000L) / int32_t(elapsed));
    }
  }
  lastPressureSampleMs = now;
}

void triggerUltrasonic() {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  triggerStartUs = micros();
  echoState = EchoState::WaitRise;
}

void failUltrasonic() {
  ultrasonicValid = false;
  ultrasonicEchoUs = 0;
  ultrasonicDistanceMm = -1;
  echoState = EchoState::Idle;
}

void updateUltrasonic(uint32_t nowMs) {
  const uint32_t nowUs = micros();
  if (echoState == EchoState::Idle && nowMs - lastUltrasonicMs >= ULTRASONIC_INTERVAL_MS) {
    lastUltrasonicMs = nowMs;
    triggerUltrasonic();
    return;
  }

  if (echoState == EchoState::WaitRise) {
    if (digitalRead(PIN_US_ECHO) == HIGH) {
      echoRiseUs = nowUs;
      echoState = EchoState::WaitFall;
    } else if (nowUs - triggerStartUs >= ULTRASONIC_TIMEOUT_US) {
      failUltrasonic();
    }
    return;
  }

  if (echoState == EchoState::WaitFall) {
    if (digitalRead(PIN_US_ECHO) == LOW) {
      const uint32_t echoUs = nowUs - echoRiseUs;
      const int16_t distanceMm = int16_t((echoUs * 10UL) / 58UL);
      int16_t depth = WATER_REFERENCE_MM - distanceMm;
      if (depth < 0) {
        depth = 0;
      }
      if (depth > WATER_MAX_MM) {
        depth = WATER_MAX_MM;
      }
      ultrasonicEchoUs = echoUs;
      ultrasonicDistanceMm = distanceMm;
      waterMm = depth;
      ultrasonicValid = true;
      lastUltrasonicOkMs = nowMs;
      faults &= ~FAULT_ULTRASONIC;
      echoState = EchoState::Idle;
    } else if (nowUs - triggerStartUs >= ULTRASONIC_TIMEOUT_US) {
      failUltrasonic();
    }
  }

  const bool neverValidTimedOut = lastUltrasonicOkMs == 0 && nowMs > FAULT_TIMEOUT_MS;
  const bool staleValidTimedOut = lastUltrasonicOkMs != 0 && nowMs - lastUltrasonicOkMs > FAULT_TIMEOUT_MS;
  if (!ultrasonicValid && (neverValidTimedOut || staleValidTimedOut)) {
    faults |= FAULT_ULTRASONIC;
  }
}

void computeTriScores() {
  waterScore = clampScore((int32_t(waterMm) * 100L) / WATER_WAIST_MM);
  const int16_t fallingSlope = pressureSlopeTenthHpaPerMin < 0 ? -pressureSlopeTenthHpaPerMin : 0;
  slopeScore = clampScore((int32_t(fallingSlope) * 100L) / SEVERE_SLOPE_TENTH_HPA_PER_MIN);
  const int16_t pressureDrop = baselineTenthHpa - pressureTenthHpa;
  baselineScore = clampScore((int32_t(pressureDrop) * 100L) / SEVERE_DROP_TENTH_HPA);
  const ProfileConfig &cfg = currentProfile();
  tri = uint8_t((uint16_t(cfg.waterWeight) * waterScore + uint16_t(cfg.slopeWeight) * slopeScore + uint16_t(cfg.baselineWeight) * baselineScore) / 100);
}

void updateAlertState(uint32_t now) {
  computeTriScores();

  warning2Condition = waterMm >= WATER_KNEE_MM;
  if (warning2Condition) {
    if (warning2SinceMs == 0) {
      warning2SinceMs = now;
    }
  } else {
    warning2SinceMs = 0;
  }

  safeCondition = ultrasonicValid && tri < TRI_WATCH_EXIT && waterMm < WATER_ANKLE_MM && pressureTenthHpa >= baselineTenthHpa - 20;
  if (safeCondition) {
    if (safeSinceMs == 0) {
      safeSinceMs = now;
    }
  } else {
    safeSinceMs = 0;
  }

  if (waterMm >= WATER_WAIST_MM) {
    alertState = AlertState::Evacuate;
  } else if ((warning2Condition && now - warning2SinceMs >= POWER_CUT_CONFIRM_MS) || relayCutLatched) {
    alertState = AlertState::Warning2PowerCut;
  } else if (tri >= TRI_WARNING1_ENTER || waterMm >= WATER_ANKLE_MM) {
    alertState = AlertState::Warning1;
  } else if (tri >= TRI_WATCH_ENTER || (alertState == AlertState::Warning1 && (tri >= TRI_WARNING1_EXIT || waterMm >= WATER_ANKLE_MM))) {
    alertState = AlertState::Watch;
  } else if (tri < TRI_WATCH_EXIT) {
    alertState = AlertState::Normal;
  }

  if (alertState == AlertState::Warning2PowerCut || alertState == AlertState::Evacuate) {
    if (!relayCutLatched) {
      setRelayCut(true);
      logEvent(EVENT_RELAY_CUT);
    }
  }

  if (alertState != previousAlertState) {
    previousAlertState = alertState;
    buzzerMuted = false;
    logEvent(EVENT_STATE);
  }
}

void updateIndicators() {
  uint8_t bars = 0;
  if (tri >= 1) {
    bars = 1;
  }
  if (tri >= 25) {
    bars = 2;
  }
  if (tri >= 50) {
    bars = 3;
  }
  if (tri >= 70) {
    bars = 4;
  }
  if (tri >= 85) {
    bars = 5;
  }

  for (uint8_t i = 0; i < sizeof(LED_PINS); ++i) {
    digitalWrite(LED_PINS[i], i < bars ? HIGH : LOW);
  }
  digitalWrite(PIN_LED_ALERT, alertState == AlertState::Warning2PowerCut || alertState == AlertState::Evacuate || relayCutLatched ? HIGH : LOW);
}

void updateBuzzer(uint32_t now) {
  if (actionBeepStartedMs != 0) {
    if (now - actionBeepStartedMs < BUZZER_ACTION_MS) {
      digitalWrite(PIN_BUZZER, HIGH);
      return;
    }
    actionBeepStartedMs = 0;
  }

  if (buzzerMuted || alertState == AlertState::Normal) {
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }

  uint16_t onMs = BUZZER_WATCH_ON_MS;
  uint16_t periodMs = BUZZER_WATCH_PERIOD_MS;
  if (alertState == AlertState::Warning1) {
    onMs = BUZZER_WARNING1_ON_MS;
    periodMs = BUZZER_WARNING1_PERIOD_MS;
  } else if (alertState == AlertState::Warning2PowerCut) {
    onMs = BUZZER_WARNING2_ON_MS;
    periodMs = BUZZER_WARNING2_PERIOD_MS;
  } else if (alertState == AlertState::Evacuate) {
    onMs = BUZZER_EVAC_ON_MS;
    periodMs = BUZZER_EVAC_PERIOD_MS;
  }

  digitalWrite(PIN_BUZZER, now % periodMs < onMs ? HIGH : LOW);
}

void cycleLcdPage() {
  ++lcdPage;
  if (lcdPage >= 5) {
    lcdPage = 0;
  }
}

bool canResetRelay(uint32_t now) {
  return safeCondition && safeSinceMs != 0 && now - safeSinceMs >= SAFE_CLEAR_MS;
}

void acknowledge(uint32_t now) {
  buzzerMuted = true;
  startActionBeep(now);
  logEvent(EVENT_ACK);
  if (relayCutLatched && canResetRelay(now)) {
    manualCutLatched = false;
    setRelayCut(false);
    alertState = AlertState::Normal;
    previousAlertState = alertState;
    logEvent(EVENT_RESET);
  }
}

void restoreRelay(uint32_t now) {
  manualCutLatched = false;
  setRelayCut(false);
  alertState = AlertState::Normal;
  previousAlertState = alertState;
  buzzerMuted = true;
  startActionBeep(now);
  logEvent(EVENT_RESET);
}

void manualCut(uint32_t now) {
  manualCutLatched = true;
  setRelayCut(true);
  alertState = AlertState::Warning2PowerCut;
  previousAlertState = alertState;
  startActionBeep(now);
  logEvent(EVENT_MANUAL_CUT);
}

void setProfile(Profile next, uint32_t now) {
  profile = next;
  startActionBeep(now);
  logEvent(EVENT_PROFILE);
}

void handleButton(ButtonId button, uint32_t now) {
  if (button == ButtonId::YellowCut) {
    manualCut(now);
  } else if (button == ButtonId::BlueView) {
    cycleLcdPage();
    startActionBeep(now);
  } else if (button == ButtonId::GreenAck) {
    acknowledge(now);
  }
}

void updateButtons(uint32_t now) {
  stableButton = ButtonId::None;
  pendingButton = ButtonId::None;
  faults &= ~FAULT_BUTTON;
  (void)now;
  return;

  if (now - lastButtonMs < BUTTON_INTERVAL_MS) {
    return;
  }
  lastButtonMs = now;

  const ButtonId rawButton = decodeButton(readButtonsAdc());
  if (rawButton != pendingButton) {
    pendingButton = rawButton;
    buttonPendingSinceMs = now;
  }

  if (now - buttonPendingSinceMs >= BUTTON_DEBOUNCE_MS && stableButton != pendingButton) {
    stableButton = pendingButton;
    if (stableButton == ButtonId::Ambiguous) {
      faults |= FAULT_BUTTON;
      logEvent(EVENT_FAULT);
    } else {
      faults &= ~FAULT_BUTTON;
      if (stableButton != ButtonId::None) {
        handleButton(stableButton, now);
      }
    }
  }
}

void handleKey(char key, uint32_t now) {
  if (key == 'A') {
    restoreRelay(now);
  } else if (key == 'B' || key == '0') {
    manualCut(now);
  } else if (key == 'C') {
    buzzerMuted = true;
    startActionBeep(now);
  } else if (key == 'D') {
    cycleLcdPage();
    startActionBeep(now);
  } else if (key == '1') {
    setProfile(Profile::Habagat, now);
  } else if (key == '2') {
    setProfile(Profile::Bagyo, now);
  } else if (key == '3') {
    setProfile(Profile::FlashFlood, now);
  } else if (key == '*') {
    restoreRelay(now);
  } else if (key == '#') {
    logEvent(EVENT_SMOKE);
    startActionBeep(now);
  }
}

void updateKeypad(uint32_t now) {
  if (now - lastKeypadMs < KEYPAD_INTERVAL_MS) {
    return;
  }
  lastKeypadMs = now;

  const char key = scanKeypad();
  if (key != 0 && key != lastKey) {
    handleKey(key, now);
  }
  lastKey = key;
}

void handleSerial(uint32_t now) {
  while (Serial.available()) {
    const char command = Serial.read();
    if (command == 'c') {
      manualCut(now);
    } else if (command == 'a') {
      restoreRelay(now);
    } else if (command == 'r') {
      restoreRelay(now);
    } else if (command == 'v') {
      cycleLcdPage();
      startActionBeep(now);
    } else if (command == 'm') {
      buzzerMuted = true;
    } else if (command == '1') {
      setProfile(Profile::Habagat, now);
    } else if (command == '2') {
      setProfile(Profile::Bagyo, now);
    } else if (command == '3') {
      setProfile(Profile::FlashFlood, now);
    } else if (command == 'e') {
      Serial.print(F("event_log_smoke="));
      Serial.println(eventLogWrite(EVENT_SMOKE, secondsOfDay(), uint8_t(alertState), uint8_t(profile), tri, waterMm, pressureTenthHpa - baselineTenthHpa, bmeTempTenthC, bmeHumidityPct, faults) ? F("ok") : F("fail"));
    } else if (command == '?') {
      Serial.println(F("commands: c cut, a/r restore, v view, m mute, 1 habagat, 2 bagyo, 3 flash, e log, ? menu"));
      Serial.println(F("keypad: B/0 cut, A/* restore, C mute, D view, 1/2/3 profile, # log"));
      Serial.println(F("panel buttons disabled; use keypad A/B/C/D"));
    }
  }
}

void drawLcd(uint32_t now) {
  if (!lcdOk || now - lastLcdMs < LCD_INTERVAL_MS) {
    return;
  }
  lastLcdMs = now;

  char line[21];

  if (lcdPage == 0) {
    lcdSetCursor(0, 0);
    snprintf(line, sizeof(line), "BShield %s", relayCutLatched ? "CUT" : "READY");
    lcdPrintFixed(line);
    lcdSetCursor(0, 1);
    snprintf(line, sizeof(line), "TRI:%3u %s", tri, stateName(alertState));
    lcdPrintFixed(line);
    lcdSetCursor(0, 2);
    snprintf(line, sizeof(line), "Water:%3dmm %s", waterMm, ultrasonicValid ? "OK" : "NO");
    lcdPrintFixed(line);
    lcdSetCursor(0, 3);
    snprintf(line, sizeof(line), "Profile:%s", currentProfile().name);
    lcdPrintFixed(line);
    return;
  }

  if (lcdPage == 1) {
    lcdSetCursor(0, 0);
    lcdPrintFixed("Water / distance");
    lcdSetCursor(0, 1);
    snprintf(line, sizeof(line), "Depth:%3dmm", waterMm);
    lcdPrintFixed(line);
    lcdSetCursor(0, 2);
    snprintf(line, sizeof(line), "Dist:%4dmm", ultrasonicDistanceMm);
    lcdPrintFixed(line);
    lcdSetCursor(0, 3);
    snprintf(line, sizeof(line), "Echo:%5luus", ultrasonicEchoUs);
    lcdPrintFixed(line);
    return;
  }

  if (lcdPage == 2) {
    lcdSetCursor(0, 0);
    lcdPrintFixed("Pressure demo");
    lcdSetCursor(0, 1);
    snprintf(line, sizeof(line), "P:%4d.%1dhPa", pressureTenthHpa / 10, abs(pressureTenthHpa % 10));
    lcdPrintFixed(line);
    lcdSetCursor(0, 2);
    snprintf(line, sizeof(line), "Slope:%4d.%1d/m", pressureSlopeTenthHpaPerMin / 10, abs(pressureSlopeTenthHpaPerMin % 10));
    lcdPrintFixed(line);
    lcdSetCursor(0, 3);
    snprintf(line, sizeof(line), "W%u S%u B%u", waterScore, slopeScore, baselineScore);
    lcdPrintFixed(line);
    return;
  }

  if (lcdPage == 3) {
    lcdSetCursor(0, 0);
    lcdPrintFixed("Env context only");
    lcdSetCursor(0, 1);
    snprintf(line, sizeof(line), "BME:%s %4d.%1d", bmeOk ? "OK" : "NO", bmePressureTenthHpa / 10, abs(bmePressureTenthHpa % 10));
    lcdPrintFixed(line);
    lcdSetCursor(0, 2);
    snprintf(line, sizeof(line), "T:%3d.%1d H:%3u%%", bmeTempTenthC / 10, abs(bmeTempTenthC % 10), bmeHumidityPct);
    lcdPrintFixed(line);
    lcdSetCursor(0, 3);
    snprintf(line, sizeof(line), "%02u:%02u:%02u F:%02X", rtcHours, rtcMinutes, rtcSeconds, faults);
    lcdPrintFixed(line);
    return;
  }

  lcdSetCursor(0, 0);
  lcdPrintFixed("Device status");
  lcdSetCursor(0, 1);
  snprintf(line, sizeof(line), "LCD:%s KEY:%s", lcdOk ? "OK" : "NO", keypadOk ? "OK" : "NO");
  lcdPrintFixed(line);
  lcdSetCursor(0, 2);
  snprintf(line, sizeof(line), "RTC:%s LOG:%s", rtcOk ? "OK" : "NO", logOk ? "OK" : "NO");
  lcdPrintFixed(line);
  lcdSetCursor(0, 3);
  snprintf(line, sizeof(line), "Reset:%s", canResetRelay(now) ? "READY" : "WAIT/UNSAFE");
  lcdPrintFixed(line);
}

void printStatus(uint32_t now) {
  if (now - lastSerialMs < SERIAL_INTERVAL_MS) {
    return;
  }
  lastSerialMs = now;

  Serial.print(F("mode=DEMO profile="));
  Serial.print(currentProfile().name);
  Serial.print(F(" tri="));
  Serial.print(tri);
  Serial.print(F(" state="));
  Serial.print(stateName(alertState));
  Serial.print(F(" water_mm="));
  Serial.print(ultrasonicValid ? waterMm : -1);
  Serial.print(F(" distance_mm="));
  Serial.print(ultrasonicValid ? ultrasonicDistanceMm : -1);
  Serial.print(F(" echo_us="));
  Serial.print(ultrasonicValid ? ultrasonicEchoUs : 0);
  Serial.print(F(" pressure_hpa="));
  printTenth(pressureTenthHpa);
  Serial.print(F(" slope_hpa_min="));
  printTenth(pressureSlopeTenthHpaPerMin);
  Serial.print(F(" bme="));
  Serial.print(bmeOk ? F("ok") : F("fault"));
  Serial.print(F(" relay="));
  Serial.print(relayCutLatched ? F("cut") : F("normal"));
  Serial.print(F(" button="));
  Serial.print(buttonName(stableButton));
  Serial.print(F(" faults=0x"));
  printHexByte(faults);
  Serial.print(F(" log_next="));
  Serial.println(eventLogNextIndex());
}

void setupI2cDevices() {
  if (i2cPresent(LCD_ADDR_PRIMARY)) {
    lcdAddress = LCD_ADDR_PRIMARY;
    lcdOk = true;
  } else if (i2cPresent(LCD_ADDR_FALLBACK)) {
    lcdAddress = LCD_ADDR_FALLBACK;
    lcdOk = true;
  } else {
    faults |= FAULT_LCD;
  }

  if (lcdOk) {
    lcdInit();
  }

  keypadOk = i2cPresent(PCF8574_ADDR);
  if (keypadOk) {
    pcfWrite(0xFF);
  } else {
    faults |= FAULT_KEYPAD;
  }

  rtcOk = i2cPresent(DS3231_ADDR) && readRtcTime();
  if (!rtcOk) {
    faults |= FAULT_RTC;
  }

  logOk = eventLogBegin(AT24C32_ADDR);
  if (!logOk) {
    faults |= FAULT_LOG;
  }

  if (bme.begin(BME280_ADDR, &Wire) && uint8_t(bme.sensorID()) == BME280_EXPECTED_ID) {
    bmeOk = true;
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::FILTER_OFF,
                    Adafruit_BME280::STANDBY_MS_1000);
  } else {
    faults |= FAULT_BME;
  }
}

void printStartup() {
  Serial.println(F("BahayShield ULTRA production firmware"));
  Serial.print(F("lcd="));
  Serial.print(lcdOk ? F("ok addr=0x") : F("missing addr=0x"));
  printHexByte(lcdAddress);
  Serial.println();
  Serial.print(F("keypad_pcf8574_0x20="));
  Serial.println(keypadOk ? F("ok") : F("missing"));
  Serial.print(F("ds3231_0x68="));
  Serial.println(rtcOk ? F("ok") : F("missing"));
  Serial.print(F("at24c32_0x57="));
  Serial.println(logOk ? F("ok") : F("missing"));
  Serial.print(F("bme280_0x76_id60="));
  Serial.println(bmeOk ? F("ok") : F("missing"));
  Serial.println(F("commands: c cut, a ack/reset-if-safe, v view, m mute, 1/2/3 profile, e log, ? menu"));
}

void setup() {
  digitalWrite(PIN_RELAY1, RELAY_INACTIVE_LEVEL);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE_LEVEL);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  digitalWrite(PIN_RELAY1, RELAY_INACTIVE_LEVEL);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE_LEVEL);

  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  pinMode(PIN_LED_G1, OUTPUT);
  pinMode(PIN_LED_G2, OUTPUT);
  pinMode(PIN_LED_Y, OUTPUT);
  pinMode(PIN_LED_O, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_US_TRIG, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  for (uint8_t i = 0; i < sizeof(LED_PINS); ++i) {
    digitalWrite(LED_PINS[i], LOW);
  }
  digitalWrite(PIN_LED_ALERT, LOW);

  Serial.begin(SERIAL_BAUD);
  Wire.begin();
  Wire.setWireTimeout(25000, true);

  const uint32_t bootNow = millis();
  pressureTenthHpa = readPressureTenthHpa();
  previousPressureTenthHpa = pressureTenthHpa;
  lastPressureSampleMs = bootNow;
  lastPressureMs = bootNow;
  setupI2cDevices();
  updateBme(BME_INTERVAL_MS);
  updateRtc(RTC_INTERVAL_MS);
  logEvent(EVENT_BOOT);
  printStartup();
}

void loop() {
  const uint32_t now = millis();

  updatePressure(now);
  updateUltrasonic(now);
  updateKeypad(now);
  updateBme(now);
  updateRtc(now);

  if (now - lastTriMs >= TRI_INTERVAL_MS) {
    lastTriMs = now;
    updateAlertState(now);
    updateIndicators();
  }

  setRelayCut(relayCutLatched);
  updateBuzzer(now);
  handleSerial(now);
  drawLcd(now);
  printStatus(now);

  if (faults != previousFaults) {
    previousFaults = faults;
    logEvent(EVENT_FAULT);
  }
}
