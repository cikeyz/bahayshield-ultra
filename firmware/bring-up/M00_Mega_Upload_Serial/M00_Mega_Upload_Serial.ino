constexpr unsigned long BLINK_INTERVAL_MS = 500;
constexpr unsigned long PRINT_INTERVAL_MS = 2000;

bool ledState = false;
unsigned long lastBlinkMs = 0;
unsigned long lastPrintMs = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println(F("BahayShield G2 M00 Mega upload and serial OK"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastBlinkMs >= BLINK_INTERVAL_MS) {
    lastBlinkMs = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  }

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    Serial.print(F("uptime_ms="));
    Serial.println(now);
  }
}
