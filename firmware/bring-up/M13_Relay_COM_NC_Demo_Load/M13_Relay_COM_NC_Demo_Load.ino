constexpr byte RELAY1_PIN = 4;
constexpr byte RELAY_ACTIVE_LEVEL = LOW;
constexpr byte RELAY_INACTIVE_LEVEL = HIGH;
constexpr unsigned long AUTO_INTERVAL_MS = 5000;

bool relayActive = false;
bool autoMode = false;
unsigned long lastAutoMs = 0;

void applyRelay() {
  digitalWrite(RELAY1_PIN, relayActive ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
  Serial.print(F("relay1="));
  Serial.print(relayActive ? F("ACTIVE") : F("INACTIVE"));
  Serial.print(F(" expected_demo_load="));
  Serial.println(relayActive ? F("OFF_COM_NC_OPEN") : F("ON_COM_NC_CLOSED"));
}

void setRelay(bool nextActive) {
  relayActive = nextActive;
  applyRelay();
}

void printMenu() {
  Serial.println(F("commands:"));
  Serial.println(F("n = normal, relay inactive, COM-NC closed, demo load ON"));
  Serial.println(F("c = cut, relay active, COM-NC open, demo load OFF"));
  Serial.println(F("t = toggle once"));
  Serial.println(F("r = repeat every 5 seconds"));
  Serial.println(F("s = stop repeat and return to normal"));
  Serial.println(F("? = print menu"));
}

void setup() {
  digitalWrite(RELAY1_PIN, RELAY_INACTIVE_LEVEL);
  pinMode(RELAY1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, RELAY_INACTIVE_LEVEL);

  Serial.begin(115200);
  Serial.println(F("BahayShield G2 M13 Relay 1 COM-NC demo load"));
  Serial.println(F("M12 proved active LOW, so defaults are already correct"));
  Serial.println(F("use low-voltage load only"));
  printMenu();
  applyRelay();
}

void loop() {
  const unsigned long now = millis();

  if (Serial.available()) {
    const char command = Serial.read();

    switch (command) {
      case 'n':
        autoMode = false;
        setRelay(false);
        break;
      case 'c':
        autoMode = false;
        setRelay(true);
        break;
      case 't':
        autoMode = false;
        setRelay(!relayActive);
        break;
      case 'r':
        autoMode = true;
        lastAutoMs = now;
        Serial.println(F("repeat=ON"));
        break;
      case 's':
        autoMode = false;
        Serial.println(F("repeat=OFF"));
        setRelay(false);
        break;
      case '?':
        printMenu();
        break;
    }
  }

  if (autoMode && now - lastAutoMs >= AUTO_INTERVAL_MS) {
    lastAutoMs = now;
    relayActive = !relayActive;
    applyRelay();
  }
}
