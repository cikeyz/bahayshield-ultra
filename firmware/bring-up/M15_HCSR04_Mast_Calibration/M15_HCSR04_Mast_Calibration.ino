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

constexpr byte RELAY_INACTIVE = HIGH;

constexpr unsigned long BAUD_RATE = 115200;
constexpr unsigned long PRINT_INTERVAL_MS = 1000;
constexpr unsigned long SAMPLE_INTERVAL_MS = 250;
constexpr unsigned long ECHO_TIMEOUT_US = 30000;
constexpr byte WINDOW_SIZE = 9;
constexpr byte MARK_COUNT = 5;
constexpr int MAX_WATER_MM = 70;
constexpr int STABLE_SPREAD_WARN_MM = 6;
constexpr int CLUSTER_WIDTH_MM = 10;

const int MARK_WATER_MM[MARK_COUNT] = {0, 15, 30, 45, 55};
const char *const MARK_NAME[MARK_COUNT] = {"dry", "ankle", "knee", "waist", "critical"};

unsigned long lastPrintMs = 0;
unsigned long lastSampleMs = 0;
unsigned long sampleCount = 0;
unsigned long timeoutCount = 0;

int samples[WINDOW_SIZE];
byte sampleIndex = 0;
byte sampleFill = 0;

int markDistanceMm[MARK_COUNT] = {-1, -1, -1, -1, -1};
int referenceMm = -1;
int lastDistanceMm = -1;
int lastMedianMm = -1;
int lastSpreadMm = -1;
int lastRobustMm = -1;
int lastClusterSpreadMm = -1;
byte lastClusterCount = 0;

unsigned long readEchoUs() {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  return pulseIn(PIN_US_ECHO, HIGH, ECHO_TIMEOUT_US);
}

int echoUsToMm(unsigned long echoUs) {
  return int((echoUs * 10UL) / 58UL);
}

void sortSmall(int *values, byte count) {
  for (byte i = 1; i < count; ++i) {
    const int value = values[i];
    byte j = i;
    while (j > 0 && values[j - 1] > value) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
}

bool getWindowStats(int &medianMm, int &spreadMm) {
  if (sampleFill == 0) {
    return false;
  }

  int sorted[WINDOW_SIZE];
  for (byte i = 0; i < sampleFill; ++i) {
    sorted[i] = samples[i];
  }
  sortSmall(sorted, sampleFill);
  medianMm = sorted[sampleFill / 2];
  spreadMm = sorted[sampleFill - 1] - sorted[0];
  return true;
}

bool getClusterStats(int &robustMm, int &clusterSpreadMm, byte &clusterCount) {
  if (sampleFill == 0) {
    return false;
  }

  int sorted[WINDOW_SIZE];
  for (byte i = 0; i < sampleFill; ++i) {
    sorted[i] = samples[i];
  }
  sortSmall(sorted, sampleFill);

  byte bestStart = 0;
  byte bestEnd = 0;
  byte start = 0;
  for (byte end = 0; end < sampleFill; ++end) {
    while (sorted[end] - sorted[start] > CLUSTER_WIDTH_MM && start < end) {
      ++start;
    }
    const byte currentCount = byte(end - start + 1);
    const byte bestCount = byte(bestEnd - bestStart + 1);
    const int currentSpread = sorted[end] - sorted[start];
    const int bestSpread = sorted[bestEnd] - sorted[bestStart];
    if (currentCount > bestCount || (currentCount == bestCount && currentSpread < bestSpread)) {
      bestStart = start;
      bestEnd = end;
    }
  }

  clusterCount = byte(bestEnd - bestStart + 1);
  clusterSpreadMm = sorted[bestEnd] - sorted[bestStart];
  robustMm = sorted[bestStart + (clusterCount / 2)];
  return true;
}

void pushSample(int distanceMm) {
  samples[sampleIndex] = distanceMm;
  sampleIndex = byte((sampleIndex + 1) % WINDOW_SIZE);
  if (sampleFill < WINDOW_SIZE) {
    ++sampleFill;
  }
  lastDistanceMm = distanceMm;
  getWindowStats(lastMedianMm, lastSpreadMm);
  getClusterStats(lastRobustMm, lastClusterSpreadMm, lastClusterCount);
}

void clearSamples() {
  sampleIndex = 0;
  sampleFill = 0;
  lastDistanceMm = -1;
  lastMedianMm = -1;
  lastSpreadMm = -1;
  lastRobustMm = -1;
  lastClusterSpreadMm = -1;
  lastClusterCount = 0;
}

void printMenu() {
  Serial.println(F("commands: d dry, 1 15mm, 2 30mm, 3 45mm, 4 55mm, r reset, t table, ? menu"));
  Serial.println(F("use robust_mm when cluster_count is 6 or more and cluster_spread_mm is small"));
}

void printCalibrationTable() {
  Serial.println(F("calibration_table"));
  for (byte i = 0; i < MARK_COUNT; ++i) {
    Serial.print(F("mark="));
    Serial.print(MARK_NAME[i]);
    Serial.print(F(" water_mm="));
    Serial.print(MARK_WATER_MM[i]);
    Serial.print(F(" measured_distance_mm="));
    Serial.print(markDistanceMm[i]);
    Serial.print(F(" expected_from_dry_mm="));
    if (markDistanceMm[0] >= 0) {
      Serial.print(markDistanceMm[0] - MARK_WATER_MM[i]);
    } else {
      Serial.print(F("na"));
    }
    Serial.print(F(" error_mm="));
    if (markDistanceMm[0] >= 0 && markDistanceMm[i] >= 0) {
      Serial.print(markDistanceMm[i] - (markDistanceMm[0] - MARK_WATER_MM[i]));
    } else {
      Serial.print(F("na"));
    }
    Serial.println();
  }
  if (markDistanceMm[0] >= 0) {
    Serial.print(F("recommended_config WATER_REFERENCE_MM="));
    Serial.print(markDistanceMm[0]);
    Serial.print(F(" WATER_MAX_MM="));
    Serial.println(MAX_WATER_MM);
  }
}

void saveMark(byte index) {
  if (lastRobustMm < 0) {
    Serial.println(F("save_failed=no_valid_distance"));
    return;
  }

  markDistanceMm[index] = lastRobustMm;
  if (index == 0) {
    referenceMm = lastMedianMm;
  } else if (referenceMm < 0 && markDistanceMm[0] >= 0) {
    referenceMm = markDistanceMm[0];
  }

  Serial.print(F("saved mark="));
  Serial.print(MARK_NAME[index]);
  Serial.print(F(" water_mm="));
  Serial.print(MARK_WATER_MM[index]);
  Serial.print(F(" distance_mm="));
  Serial.print(markDistanceMm[index]);
  Serial.print(F(" cluster_count="));
  Serial.print(lastClusterCount);
  Serial.print(F(" cluster_spread_mm="));
  Serial.println(lastClusterSpreadMm);
  printCalibrationTable();
}

void handleSerial() {
  while (Serial.available()) {
    const char command = Serial.read();
    if (command == 'd' || command == '0') {
      saveMark(0);
    } else if (command == '1') {
      saveMark(1);
    } else if (command == '2') {
      saveMark(2);
    } else if (command == '3') {
      saveMark(3);
    } else if (command == '4') {
      saveMark(4);
    } else if (command == 'r') {
      for (byte i = 0; i < MARK_COUNT; ++i) {
        markDistanceMm[i] = -1;
      }
      referenceMm = -1;
      clearSamples();
      Serial.println(F("calibration_reset"));
    } else if (command == 't') {
      printCalibrationTable();
    } else if (command == '?') {
      printMenu();
    }
  }
}

void printStatus(unsigned long now) {
  if (now - lastPrintMs < PRINT_INTERVAL_MS) {
    return;
  }
  lastPrintMs = now;

  int computedWaterMm = -1;
  if (lastRobustMm >= 0 && referenceMm >= 0) {
    computedWaterMm = referenceMm - lastRobustMm;
    if (computedWaterMm < 0) {
      computedWaterMm = 0;
    }
    if (computedWaterMm > MAX_WATER_MM) {
      computedWaterMm = MAX_WATER_MM;
    }
  }

  Serial.print(F("m15 samples="));
  Serial.print(sampleCount);
  Serial.print(F(" timeouts="));
  Serial.print(timeoutCount);
  Serial.print(F(" latest_mm="));
  Serial.print(lastDistanceMm);
  Serial.print(F(" median_mm="));
  Serial.print(lastMedianMm);
  Serial.print(F(" robust_mm="));
  Serial.print(lastRobustMm);
  Serial.print(F(" spread_mm="));
  Serial.print(lastSpreadMm);
  Serial.print(F(" cluster_count="));
  Serial.print(lastClusterCount);
  Serial.print(F(" cluster_spread_mm="));
  Serial.print(lastClusterSpreadMm);
  Serial.print(F(" reference_mm="));
  Serial.print(referenceMm);
  Serial.print(F(" water_mm="));
  Serial.print(computedWaterMm);
  Serial.print(F(" stable="));
  Serial.println(lastClusterCount >= 6 && lastClusterSpreadMm >= 0 && lastClusterSpreadMm <= STABLE_SPREAD_WARN_MM ? F("yes") : F("no"));
}

void sampleUltrasonic(unsigned long now) {
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs = now;

  const unsigned long echoUs = readEchoUs();
  if (echoUs == 0) {
    ++timeoutCount;
    return;
  }

  ++sampleCount;
  pushSample(echoUsToMm(echoUs));
}

void setup() {
  Serial.begin(BAUD_RATE);
  digitalWrite(PIN_RELAY1, RELAY_INACTIVE);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE);
  digitalWrite(PIN_LED_G1, LOW);
  digitalWrite(PIN_LED_G2, LOW);
  digitalWrite(PIN_LED_Y, LOW);
  digitalWrite(PIN_LED_O, LOW);
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_ALERT, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_LED_G1, OUTPUT);
  pinMode(PIN_LED_G2, OUTPUT);
  pinMode(PIN_LED_Y, OUTPUT);
  pinMode(PIN_LED_O, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  Serial.println(F("BahayShield G2 M15 HC-SR04 mast calibration"));
  Serial.println(F("Mega pins: TRIG=D2 ECHO=D3"));
  printMenu();
}

void loop() {
  const unsigned long now = millis();
  sampleUltrasonic(now);
  handleSerial();
  printStatus(now);
}
