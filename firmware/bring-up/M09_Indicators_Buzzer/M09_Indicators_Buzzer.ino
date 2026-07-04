constexpr byte OUTPUT_PINS[] = {6, 7, 8, 9, 10, 11, 12};
constexpr unsigned long STEP_INTERVAL_MS = 1000;
constexpr unsigned long BUZZER_ON_MS = 180;

byte stepIndex = 0;
unsigned long lastStepMs = 0;

void allOff() {
  for (byte i = 0; i < sizeof(OUTPUT_PINS); ++i) {
    digitalWrite(OUTPUT_PINS[i], LOW);
  }
}

void setup() {
  Serial.begin(115200);

  for (byte i = 0; i < sizeof(OUTPUT_PINS); ++i) {
    digitalWrite(OUTPUT_PINS[i], LOW);
    pinMode(OUTPUT_PINS[i], OUTPUT);
  }

  Serial.println(F("BahayShield G2 M09 indicators and buzzer"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastStepMs >= STEP_INTERVAL_MS) {
    lastStepMs = now;
    allOff();
    digitalWrite(OUTPUT_PINS[stepIndex], HIGH);

    Serial.print(F("output_pin=D"));
    Serial.println(OUTPUT_PINS[stepIndex]);

    ++stepIndex;
    if (stepIndex >= sizeof(OUTPUT_PINS)) {
      stepIndex = 0;
    }
  }

  if (OUTPUT_PINS[stepIndex == 0 ? sizeof(OUTPUT_PINS) - 1 : stepIndex - 1] == 12 && now - lastStepMs >= BUZZER_ON_MS) {
    digitalWrite(12, LOW);
  }
}
