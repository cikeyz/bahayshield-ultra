#ifndef BAHAYSHIELD_PINS_H
#define BAHAYSHIELD_PINS_H

#include <Arduino.h>

constexpr uint8_t PIN_US_TRIG = 2;
constexpr uint8_t PIN_US_ECHO = 3;
constexpr uint8_t PIN_RELAY1 = 4;
constexpr uint8_t PIN_RELAY2 = 5;
constexpr uint8_t PIN_LED_G1 = 6;
constexpr uint8_t PIN_LED_G2 = 7;
constexpr uint8_t PIN_LED_Y = 8;
constexpr uint8_t PIN_LED_O = 9;
constexpr uint8_t PIN_LED_R = 10;
constexpr uint8_t PIN_LED_ALERT = 11;
constexpr uint8_t PIN_BUZZER = 12;
constexpr uint8_t PIN_PRESSURE = A0;
constexpr uint8_t PIN_BUTTONS = A1;

constexpr uint8_t RELAY_ACTIVE_LEVEL = LOW;
constexpr uint8_t RELAY_INACTIVE_LEVEL = HIGH;

#endif
