#include <Wire.h>

constexpr byte LCD_ADDR_PRIMARY = 0x27;
constexpr byte LCD_ADDR_FALLBACK = 0x3F;
constexpr byte LCD_BACKLIGHT = 0x08;
constexpr byte LCD_ENABLE = 0x04;
constexpr byte LCD_RS = 0x01;
constexpr unsigned long REFRESH_INTERVAL_MS = 1000;

byte lcdAddress = 0;
unsigned long lastRefreshMs = 0;
uint16_t counter = 0;

bool i2cPresent(byte address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void lcdWriteExpander(byte value) {
  Wire.beginTransmission(lcdAddress);
  Wire.write(value | LCD_BACKLIGHT);
  Wire.endTransmission();
}

void lcdPulse(byte value) {
  lcdWriteExpander(value | LCD_ENABLE);
  delayMicroseconds(1);
  lcdWriteExpander(value & ~LCD_ENABLE);
  delayMicroseconds(50);
}

void lcdWrite4(byte value, byte mode) {
  lcdPulse((value & 0xF0) | mode);
}

void lcdWrite(byte value, byte mode) {
  lcdWrite4(value & 0xF0, mode);
  lcdWrite4((value << 4) & 0xF0, mode);
}

void lcdCommand(byte value) {
  lcdWrite(value, 0);
  if (value == 0x01 || value == 0x02) {
    delay(2);
  }
}

void lcdData(byte value) {
  lcdWrite(value, LCD_RS);
}

void lcdPrintText(const char *text) {
  while (*text) {
    lcdData(*text++);
  }
}

void lcdSetCursor(byte col, byte row) {
  static const byte offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcdCommand(0x80 | (col + offsets[row]));
}

void lcdClear() {
  lcdCommand(0x01);
}

void lcdInit() {
  delay(50);
  lcdWrite4(0x30, 0);
  delay(5);
  lcdWrite4(0x30, 0);
  delayMicroseconds(150);
  lcdWrite4(0x30, 0);
  lcdWrite4(0x20, 0);
  lcdCommand(0x28);
  lcdCommand(0x08);
  lcdClear();
  lcdCommand(0x06);
  lcdCommand(0x0C);
}

void printPaddedNumber(uint16_t value) {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%u", value);
  lcdPrintText(buffer);
}

void drawScreen() {
  lcdSetCursor(0, 0);
  lcdPrintText("BahayShield G2     ");
  lcdSetCursor(0, 1);
  lcdPrintText("LCD 20x4 direct OK ");
  lcdSetCursor(0, 2);
  lcdPrintText("Mega SDA20 SCL21   ");
  lcdSetCursor(0, 3);
  lcdPrintText("count=");
  printPaddedNumber(counter++);
  lcdPrintText("             ");
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (i2cPresent(LCD_ADDR_PRIMARY)) {
    lcdAddress = LCD_ADDR_PRIMARY;
  } else if (i2cPresent(LCD_ADDR_FALLBACK)) {
    lcdAddress = LCD_ADDR_FALLBACK;
  }

  if (lcdAddress == 0) {
    Serial.println(F("lcd_missing"));
    while (true) {
    }
  }

  Serial.print(F("lcd_addr=0x"));
  if (lcdAddress < 16) {
    Serial.print('0');
  }
  Serial.println(lcdAddress, HEX);

  lcdInit();
  drawScreen();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastRefreshMs >= REFRESH_INTERVAL_MS) {
    lastRefreshMs = now;
    drawScreen();
  }
}
