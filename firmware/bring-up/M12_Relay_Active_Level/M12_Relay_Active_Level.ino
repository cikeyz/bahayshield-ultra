constexpr byte RELAY1_PIN = 4;
constexpr byte RELAY2_PIN = 5;

void printMenu() {
  Serial.println(F("watch relay LEDs and listen for clicks"));
  Serial.println(F("if a relay turns on at LOW, it is active LOW"));
  Serial.println(F("if a relay turns on at HIGH, it is active HIGH"));
  Serial.println(F("commands:"));
  Serial.println(F("1 = IN1 HIGH"));
  Serial.println(F("2 = IN1 LOW"));
  Serial.println(F("3 = IN2 HIGH"));
  Serial.println(F("4 = IN2 LOW"));
  Serial.println(F("a = both HIGH"));
  Serial.println(F("b = both LOW"));
}

void setup() {
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  Serial.begin(115200);
  Serial.println(F("BahayShield G2 M12 relay active level"));
  Serial.println(F("run with relay contacts unloaded first"));
  Serial.println(F("startup_state: in1=HIGH in2=HIGH"));
  printMenu();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  const char command = Serial.read();

  switch (command) {
    case '1':
      digitalWrite(RELAY1_PIN, HIGH);
      Serial.println(F("in1=HIGH"));
      break;
    case '2':
      digitalWrite(RELAY1_PIN, LOW);
      Serial.println(F("in1=LOW"));
      break;
    case '3':
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println(F("in2=HIGH"));
      break;
    case '4':
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println(F("in2=LOW"));
      break;
    case 'a':
      digitalWrite(RELAY1_PIN, HIGH);
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println(F("in1=HIGH in2=HIGH"));
      break;
    case 'b':
      digitalWrite(RELAY1_PIN, LOW);
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println(F("in1=LOW in2=LOW"));
      break;
    case '?':
      printMenu();
      break;
  }
}
