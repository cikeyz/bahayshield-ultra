constexpr byte BUTTON_PIN = A1;
constexpr unsigned long PRINT_INTERVAL_MS = 100;

unsigned long lastPrintMs = 0;

const __FlashStringHelper *decodeButton(int adc) {
  if (adc >= 900 && adc <= 1000) {
    return F("BTN1_YELLOW_MANUAL_CUT");
  }
  if (adc >= 780 && adc <= 890) {
    return F("BTN2_BLUE_LCD_VIEW");
  }
  if (adc >= 630 && adc <= 760) {
    return F("BTN3_GREEN_ACK_RESET");
  }
  if (adc >= 0 && adc <= 80) {
    return F("NONE");
  }
  return F("AMBIGUOUS");
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("BahayShield G2 M08 A1 button ladder"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    const int adc = analogRead(BUTTON_PIN);

    Serial.print(F("a1="));
    Serial.print(adc);
    Serial.print(F(" decoded="));
    Serial.println(decodeButton(adc));
  }
}
