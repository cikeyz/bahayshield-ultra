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
constexpr unsigned long CAPTURE_DURATION_MS = 12000;
constexpr byte WINDOW_SIZE = 9;
constexpr byte CAPTURE_MAX_VALUES = 64;
constexpr byte CAPTURE_MIN_VALUES = 12;
constexpr byte MARK_COUNT = 5;
constexpr int WATER_REFERENCE_MM = 120;
constexpr int WATER_MAX_MM = 70;
constexpr int ACCEPT_ERROR_MM = 6;
constexpr int CLUSTER_WIDTH_MM = 10;
constexpr int STABLE_CLUSTER_SPREAD_MM = 6;
constexpr int CAPTURE_CLUSTER_WIDTH_MM = 8;
constexpr int CAPTURE_PASS_SPREAD_MM = 6;
constexpr byte STABLE_CLUSTER_COUNT_MIN = 6;

const byte LED_PINS[] = {PIN_LED_G1, PIN_LED_G2, PIN_LED_Y, PIN_LED_O, PIN_LED_R};
const int MARK_WATER_MM[MARK_COUNT] = {0, 15, 30, 45, 55};
const char *const MARK_NAME[MARK_COUNT] = {"dry", "ankle", "knee", "waist", "critical"};

unsigned long lastPrintMs = 0;
unsigned long lastSampleMs = 0;
unsigned long sampleCount = 0;
unsigned long timeoutCount = 0;
unsigned long captureStartMs = 0;
unsigned long captureLastProgressMs = 0;
unsigned long captureTimeoutStart = 0;

int samples[WINDOW_SIZE];
int captureValues[CAPTURE_MAX_VALUES];
int savedDistanceMm[MARK_COUNT] = {-1, -1, -1, -1, -1};
int savedDepthMm[MARK_COUNT] = {-1, -1, -1, -1, -1};
int savedErrorMm[MARK_COUNT] = {0, 0, 0, 0, 0};
int savedClusterSpreadMm[MARK_COUNT] = {-1, -1, -1, -1, -1};

byte sampleIndex = 0;
byte sampleFill = 0;
byte captureValueCount = 0;
byte captureStableWindowCount = 0;
byte captureRejectedWindowCount = 0;
byte captureMarkIndex = 0;

int lastDistanceMm = -1;
int lastMedianMm = -1;
int lastSpreadMm = -1;
int lastRobustMm = -1;
int lastClusterSpreadMm = -1;
int waterMm = -1;
byte lastClusterCount = 0;
byte alertLevel = 0;
bool captureActive = false;

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

int clampWaterDepth(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > WATER_MAX_MM) {
    return WATER_MAX_MM;
  }
  return value;
}

int absInt(int value) {
  return value < 0 ? -value : value;
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

bool getClusterStatsFromSorted(const int *sorted, byte count, int clusterWidthMm, int &robustMm, int &clusterSpreadMm, byte &clusterCount) {
  if (count == 0) {
    return false;
  }

  byte bestStart = 0;
  byte bestEnd = 0;
  byte start = 0;
  for (byte end = 0; end < count; ++end) {
    while (sorted[end] - sorted[start] > clusterWidthMm && start < end) {
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

bool getClusterStats(int &robustMm, int &clusterSpreadMm, byte &clusterCount) {
  if (sampleFill == 0) {
    return false;
  }

  int sorted[WINDOW_SIZE];
  for (byte i = 0; i < sampleFill; ++i) {
    sorted[i] = samples[i];
  }
  sortSmall(sorted, sampleFill);
  return getClusterStatsFromSorted(sorted, sampleFill, CLUSTER_WIDTH_MM, robustMm, clusterSpreadMm, clusterCount);
}

bool getCaptureStats(int &robustMm, int &clusterSpreadMm, byte &clusterCount, int &minimumMm, int &maximumMm) {
  if (captureValueCount == 0) {
    return false;
  }

  int sorted[CAPTURE_MAX_VALUES];
  for (byte i = 0; i < captureValueCount; ++i) {
    sorted[i] = captureValues[i];
  }
  sortSmall(sorted, captureValueCount);
  minimumMm = sorted[0];
  maximumMm = sorted[captureValueCount - 1];
  return getClusterStatsFromSorted(sorted, captureValueCount, CAPTURE_CLUSTER_WIDTH_MM, robustMm, clusterSpreadMm, clusterCount);
}

bool readingStable() {
  return lastClusterCount >= STABLE_CLUSTER_COUNT_MIN && lastClusterSpreadMm >= 0 && lastClusterSpreadMm <= STABLE_CLUSTER_SPREAD_MM;
}

void updateAlertLevel() {
  byte level = 0;
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
  alertLevel = level;
}

void updateIndicators() {
  for (byte i = 0; i < sizeof(LED_PINS); ++i) {
    digitalWrite(LED_PINS[i], i < alertLevel ? HIGH : LOW);
  }
  digitalWrite(PIN_LED_ALERT, waterMm >= 55 ? HIGH : LOW);
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
  waterMm = clampWaterDepth(WATER_REFERENCE_MM - lastRobustMm);
  updateAlertLevel();
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
  waterMm = -1;
  alertLevel = 0;
  updateIndicators();
}

int expectedDistanceForMark(byte index) {
  return WATER_REFERENCE_MM - MARK_WATER_MM[index];
}

void printResultTable() {
  Serial.println();
  Serial.println(F("Saved water-level table"));
  Serial.println(F("Mark      Target  Distance  Water  Error  Quality"));
  for (byte i = 0; i < MARK_COUNT; ++i) {
    Serial.print(MARK_NAME[i]);
    Serial.print(F(" target="));
    Serial.print(MARK_WATER_MM[i]);
    Serial.print(F("mm expected_dist="));
    Serial.print(expectedDistanceForMark(i));
    Serial.print(F("mm saved_dist="));
    Serial.print(savedDistanceMm[i]);
    Serial.print(F("mm saved_water="));
    Serial.print(savedDepthMm[i]);
    Serial.print(F("mm error="));
    Serial.print(savedErrorMm[i]);
    Serial.print(F("mm spread="));
    Serial.print(savedClusterSpreadMm[i]);
    Serial.print(F(" pass="));
    Serial.println(savedDistanceMm[i] >= 0 && absInt(savedErrorMm[i]) <= ACCEPT_ERROR_MM && savedClusterSpreadMm[i] <= CAPTURE_PASS_SPREAD_MM ? F("YES") : F("NO"));
  }
  Serial.println();
}

void resetSavedMarks() {
  for (byte i = 0; i < MARK_COUNT; ++i) {
    savedDistanceMm[i] = -1;
    savedDepthMm[i] = -1;
    savedErrorMm[i] = 0;
    savedClusterSpreadMm[i] = -1;
  }
}

void printNextStep(byte index) {
  if (index + 1 < MARK_COUNT) {
    Serial.print(F("Next: fill to "));
    Serial.print(MARK_WATER_MM[index + 1]);
    Serial.print(F(" mm and type "));
    Serial.println(index + 1);
  } else {
    Serial.println(F("Next: type t to print the final table."));
  }
}

void finishCapture() {
  int finalDistanceMm = -1;
  int finalClusterSpreadMm = -1;
  int minDistanceMm = -1;
  int maxDistanceMm = -1;
  byte finalClusterCount = 0;
  const byte index = captureMarkIndex;
  const unsigned long timeoutDelta = timeoutCount - captureTimeoutStart;
  const bool hasStats = getCaptureStats(finalDistanceMm, finalClusterSpreadMm, finalClusterCount, minDistanceMm, maxDistanceMm);

  captureActive = false;

  Serial.println();
  Serial.println(F("CAPTURE DONE"));
  Serial.print(F("Mark: "));
  Serial.print(MARK_NAME[index]);
  Serial.print(F(" / target water "));
  Serial.print(MARK_WATER_MM[index]);
  Serial.println(F(" mm"));

  if (!hasStats) {
    Serial.println(F("Result: FAIL - no stable readings were captured."));
    Serial.println(F("Wait for stable=yes, calm the water, then try this mark again."));
    return;
  }

  const int measuredWaterMm = clampWaterDepth(WATER_REFERENCE_MM - finalDistanceMm);
  const int errorMm = measuredWaterMm - MARK_WATER_MM[index];
  const bool enoughValues = captureValueCount >= CAPTURE_MIN_VALUES;
  const bool tightCluster = finalClusterSpreadMm <= CAPTURE_PASS_SPREAD_MM;
  const bool accurate = absInt(errorMm) <= ACCEPT_ERROR_MM;
  const bool noTimeouts = timeoutDelta == 0;
  const bool pass = enoughValues && tightCluster && accurate && noTimeouts;

  savedDistanceMm[index] = finalDistanceMm;
  savedDepthMm[index] = measuredWaterMm;
  savedErrorMm[index] = errorMm;
  savedClusterSpreadMm[index] = finalClusterSpreadMm;

  Serial.print(F("Expected distance: "));
  Serial.print(expectedDistanceForMark(index));
  Serial.println(F(" mm"));
  Serial.print(F("Measured distance: "));
  Serial.print(finalDistanceMm);
  Serial.println(F(" mm"));
  Serial.print(F("Measured water: "));
  Serial.print(measuredWaterMm);
  Serial.println(F(" mm"));
  Serial.print(F("Error: "));
  Serial.print(errorMm);
  Serial.println(F(" mm"));
  Serial.print(F("Quality: accepted_readings="));
  Serial.print(captureValueCount);
  Serial.print(F(" rejected_windows="));
  Serial.print(captureRejectedWindowCount);
  Serial.print(F(" cluster_count="));
  Serial.print(finalClusterCount);
  Serial.print(F(" cluster_spread_mm="));
  Serial.print(finalClusterSpreadMm);
  Serial.print(F(" min_mm="));
  Serial.print(minDistanceMm);
  Serial.print(F(" max_mm="));
  Serial.print(maxDistanceMm);
  Serial.print(F(" timeouts="));
  Serial.println(timeoutDelta);
  Serial.print(F("Result: "));
  Serial.println(pass ? F("PASS") : F("CHECK / RETEST"));

  if (!pass) {
    if (!enoughValues) {
      Serial.println(F("- Not enough stable readings accepted."));
    }
    if (!tightCluster) {
      Serial.println(F("- Reading cluster is too wide; calm water or clear the sensor view."));
    }
    if (!accurate) {
      Serial.println(F("- Measured level is outside the +/-6 mm target window."));
    }
    if (!noTimeouts) {
      Serial.println(F("- Timeouts occurred during capture."));
    }
  }

  printResultTable();
  printNextStep(index);
}

void printCaptureProgress(unsigned long now) {
  if (!captureActive || now - captureLastProgressMs < PRINT_INTERVAL_MS) {
    return;
  }
  captureLastProgressMs = now;

  const unsigned long elapsedMs = now - captureStartMs;
  const unsigned long remainingMs = elapsedMs >= CAPTURE_DURATION_MS ? 0 : CAPTURE_DURATION_MS - elapsedMs;

  Serial.print(F("Capturing "));
  Serial.print(MARK_NAME[captureMarkIndex]);
  Serial.print(F(" target="));
  Serial.print(MARK_WATER_MM[captureMarkIndex]);
  Serial.print(F("mm remaining_s="));
  Serial.print((remainingMs + 999UL) / 1000UL);
  Serial.print(F(" live_water_mm="));
  Serial.print(waterMm);
  Serial.print(F(" robust_dist_mm="));
  Serial.print(lastRobustMm);
  Serial.print(F(" stable_now="));
  Serial.print(readingStable() ? F("yes") : F("no"));
  Serial.print(F(" accepted="));
  Serial.print(captureValueCount);
  Serial.print(F(" rejected="));
  Serial.println(captureRejectedWindowCount);
}

void startCapture(byte index, unsigned long now) {
  if (captureActive) {
    Serial.println(F("A capture is already running. Type x to cancel it first."));
    return;
  }

  clearSamples();
  captureActive = true;
  captureMarkIndex = index;
  captureStartMs = now;
  captureLastProgressMs = 0;
  captureTimeoutStart = timeoutCount;
  captureValueCount = 0;
  captureStableWindowCount = 0;
  captureRejectedWindowCount = 0;

  Serial.println();
  Serial.println(F("CAPTURE START"));
  Serial.print(F("Mark: "));
  Serial.print(MARK_NAME[index]);
  Serial.print(F(" / target water "));
  Serial.print(MARK_WATER_MM[index]);
  Serial.println(F(" mm"));
  Serial.print(F("Expected distance from sensor: "));
  Serial.print(expectedDistanceForMark(index));
  Serial.println(F(" mm"));
  Serial.println(F("Keep the water still. Do not move the mast. Wait 12 seconds."));
}

void cancelCapture() {
  if (!captureActive) {
    Serial.println(F("No active capture."));
    return;
  }
  captureActive = false;
  Serial.println(F("Capture cancelled."));
}

void updateCaptureOnNewSample(unsigned long now) {
  if (!captureActive) {
    return;
  }

  if (readingStable() && lastRobustMm >= 0) {
    if (captureValueCount < CAPTURE_MAX_VALUES) {
      captureValues[captureValueCount] = lastRobustMm;
      ++captureValueCount;
    }
    ++captureStableWindowCount;
  } else if (sampleFill >= WINDOW_SIZE) {
    ++captureRejectedWindowCount;
  }

  if (now - captureStartMs >= CAPTURE_DURATION_MS) {
    finishCapture();
  }
}

void printMenu() {
  Serial.println();
  Serial.println(F("BahayShield M16 guided water-level test"));
  Serial.println(F("Type one command, then wait for the 12-second capture to finish:"));
  Serial.println(F("0 or d = capture dry 0 mm"));
  Serial.println(F("1      = capture ankle 15 mm"));
  Serial.println(F("2      = capture knee 30 mm"));
  Serial.println(F("3      = capture waist 45 mm"));
  Serial.println(F("4      = capture critical 55 mm"));
  Serial.println(F("t      = print saved table"));
  Serial.println(F("r      = reset saved table"));
  Serial.println(F("c      = clear live sample window"));
  Serial.println(F("x      = cancel active capture"));
  Serial.println(F("?      = show this menu"));
  Serial.println();
}

void handleSerial(unsigned long now) {
  while (Serial.available()) {
    const char command = Serial.read();
    if (command == '\n' || command == '\r' || command == ' ') {
      continue;
    }
    if (command == '0' || command == 'd') {
      startCapture(0, now);
    } else if (command == '1') {
      startCapture(1, now);
    } else if (command == '2') {
      startCapture(2, now);
    } else if (command == '3') {
      startCapture(3, now);
    } else if (command == '4') {
      startCapture(4, now);
    } else if (command == 't') {
      printResultTable();
    } else if (command == 'r') {
      resetSavedMarks();
      Serial.println(F("Saved table reset."));
    } else if (command == 'c') {
      clearSamples();
      Serial.println(F("Live sample window cleared."));
    } else if (command == 'x') {
      cancelCapture();
    } else if (command == '?') {
      printMenu();
    } else {
      Serial.println(F("Unknown command. Type ? for menu."));
    }
  }
}

bool sampleUltrasonic(unsigned long now) {
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) {
    return false;
  }
  lastSampleMs = now;

  const unsigned long echoUs = readEchoUs();
  if (echoUs == 0) {
    ++timeoutCount;
    return true;
  }

  ++sampleCount;
  pushSample(echoUsToMm(echoUs));
  return true;
}

void printLiveStatus(unsigned long now) {
  if (captureActive || now - lastPrintMs < PRINT_INTERVAL_MS) {
    return;
  }
  lastPrintMs = now;

  Serial.print(F("Live: water="));
  Serial.print(waterMm);
  Serial.print(F("mm distance="));
  Serial.print(lastRobustMm);
  Serial.print(F("mm stable="));
  Serial.print(readingStable() ? F("yes") : F("no"));
  Serial.print(F(" cluster="));
  Serial.print(lastClusterCount);
  Serial.print(F("/"));
  Serial.print(lastClusterSpreadMm);
  Serial.print(F("mm"));
  Serial.print(F(" raw_latest="));
  Serial.print(lastDistanceMm);
  Serial.print(F("mm timeouts="));
  Serial.println(timeoutCount);
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
  resetSavedMarks();
  Serial.println(F("BahayShield G3 M16 HC-SR04 guided water-level test"));
  Serial.println(F("Mega pins: TRIG=D2 ECHO=D3"));
  Serial.println(F("Reference distance: 120 mm"));
  Serial.println(F("Expected distances: dry 120, 15mm 105, 30mm 90, 45mm 75, 55mm 65"));
  printMenu();
}

void loop() {
  const unsigned long now = millis();
  const bool newSample = sampleUltrasonic(now);
  if (newSample) {
    updateCaptureOnNewSample(now);
  }
  updateIndicators();
  handleSerial(now);
  printCaptureProgress(now);
  printLiveStatus(now);
}
