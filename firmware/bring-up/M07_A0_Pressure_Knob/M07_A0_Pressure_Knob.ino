constexpr byte PRESSURE_PIN = A0;
constexpr unsigned long PRINT_INTERVAL_MS = 250;

unsigned long lastPrintMs = 0;

void printTenth(long value) {
  Serial.print(value / 10);
  Serial.print('.');
  Serial.print(abs(value % 10));
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("BahayShield G2 M07 A0 pressure knob"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    const int adc = analogRead(PRESSURE_PIN);
    const long pressureTenth = map(adc, 0, 1023, 9850, 10130);

    Serial.print(F("a0="));
    Serial.print(adc);
    Serial.print(F(" pressure_hpa="));
    printTenth(pressureTenth);
    Serial.println();
  }
}
