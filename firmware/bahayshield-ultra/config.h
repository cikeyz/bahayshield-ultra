#ifndef BAHAYSHIELD_CONFIG_H
#define BAHAYSHIELD_CONFIG_H

#include <Arduino.h>

constexpr uint32_t SERIAL_BAUD = 115200;

constexpr uint8_t LCD_ADDR_PRIMARY = 0x27;
constexpr uint8_t LCD_ADDR_FALLBACK = 0x3F;
constexpr uint8_t PCF8574_ADDR = 0x20;
constexpr uint8_t DS3231_ADDR = 0x68;
constexpr uint8_t AT24C32_ADDR = 0x57;
constexpr uint8_t BME280_ADDR = 0x76;
constexpr uint8_t BME280_EXPECTED_ID = 0x60;

constexpr uint32_t BUTTON_INTERVAL_MS = 25;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 80;
constexpr uint8_t BUTTON_ADC_SAMPLES = 12;
constexpr uint32_t KEYPAD_INTERVAL_MS = 35;
constexpr uint32_t PRESSURE_INTERVAL_MS = 1000;
constexpr uint8_t PRESSURE_ADC_SAMPLES = 16;
constexpr int16_t PRESSURE_MAX_STEP_TENTH_HPA = 8;
constexpr uint32_t BME_INTERVAL_MS = 2000;
constexpr uint32_t RTC_INTERVAL_MS = 1000;
constexpr uint32_t TRI_INTERVAL_MS = 100;
constexpr uint32_t LCD_INTERVAL_MS = 1000;
constexpr uint32_t SERIAL_INTERVAL_MS = 1000;
constexpr uint32_t ULTRASONIC_INTERVAL_MS = 100;
constexpr uint32_t ULTRASONIC_TIMEOUT_US = 30000;
constexpr uint32_t FAULT_TIMEOUT_MS = 2500;

constexpr int16_t WATER_REFERENCE_MM = 120;
constexpr int16_t WATER_MAX_MM = 70;
constexpr int16_t WATER_ANKLE_MM = 15;
constexpr int16_t WATER_KNEE_MM = 30;
constexpr int16_t WATER_WAIST_MM = 55;
constexpr uint32_t POWER_CUT_CONFIRM_MS = 30000;
constexpr uint32_t SAFE_CLEAR_MS = 30000;

constexpr int16_t PRESSURE_MIN_TENTH_HPA = 9850;
constexpr int16_t PRESSURE_MAX_TENTH_HPA = 10130;
constexpr int16_t PRESSURE_BASELINE_TENTH_HPA = 10130;
constexpr int16_t SEVERE_SLOPE_TENTH_HPA_PER_MIN = 60;
constexpr int16_t SEVERE_DROP_TENTH_HPA = 280;

constexpr uint8_t TRI_WATCH_ENTER = 40;
constexpr uint8_t TRI_WATCH_EXIT = 35;
constexpr uint8_t TRI_WARNING1_ENTER = 65;
constexpr uint8_t TRI_WARNING1_EXIT = 60;
constexpr uint8_t TRI_WARNING2_ENTER = 85;
constexpr uint8_t TRI_EVACUATE_ENTER = 95;

constexpr uint16_t BUTTON_BLUE_MIN = 45;
constexpr uint16_t BUTTON_BLUE_MAX = 75;
constexpr uint16_t BUTTON_NONE_MIN = 80;
constexpr uint16_t BUTTON_NONE_MAX = 94;
constexpr uint16_t BUTTON_GREEN_MIN = 95;
constexpr uint16_t BUTTON_GREEN_MAX = 105;
constexpr uint16_t BUTTON_YELLOW_MIN = 106;
constexpr uint16_t BUTTON_YELLOW_MAX = 130;

constexpr uint16_t BUZZER_ACTION_MS = 100;
constexpr uint16_t BUZZER_WATCH_ON_MS = 80;
constexpr uint16_t BUZZER_WATCH_PERIOD_MS = 4000;
constexpr uint16_t BUZZER_WARNING1_ON_MS = 100;
constexpr uint16_t BUZZER_WARNING1_PERIOD_MS = 1800;
constexpr uint16_t BUZZER_WARNING2_ON_MS = 150;
constexpr uint16_t BUZZER_WARNING2_PERIOD_MS = 800;
constexpr uint16_t BUZZER_EVAC_ON_MS = 250;
constexpr uint16_t BUZZER_EVAC_PERIOD_MS = 500;

#endif
