constexpr byte TRIG_PIN = 2;
constexpr byte ECHO_PIN = 3;
constexpr unsigned long PRINT_INTERVAL_MS = 500;
constexpr unsigned long ECHO_TIMEOUT_US = 30000;

unsigned long lastPrintMs = 0;

unsigned long readDistanceUs() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  Serial.println(F("BahayShield G2 M10 HC-SR04 D2/D3"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    const unsigned long echoUs = readDistanceUs();

    if (echoUs == 0) {
      Serial.println(F("distance_timeout"));
      return;
    }

    const unsigned long distanceMm = (echoUs * 10UL) / 58UL;
    Serial.print(F("echo_us="));
    Serial.print(echoUs);
    Serial.print(F(" distance_mm="));
    Serial.println(distanceMm);
  }
}
