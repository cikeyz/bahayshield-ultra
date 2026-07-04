#ifndef BAHAYSHIELD_EVENT_LOG_H
#define BAHAYSHIELD_EVENT_LOG_H

#include <Arduino.h>

constexpr uint8_t EVENT_BOOT = 1;
constexpr uint8_t EVENT_PROFILE = 2;
constexpr uint8_t EVENT_STATE = 3;
constexpr uint8_t EVENT_RELAY_CUT = 4;
constexpr uint8_t EVENT_MANUAL_CUT = 5;
constexpr uint8_t EVENT_ACK = 6;
constexpr uint8_t EVENT_RESET = 7;
constexpr uint8_t EVENT_FAULT = 8;
constexpr uint8_t EVENT_SMOKE = 9;

bool eventLogBegin(uint8_t address);
bool eventLogReady();
uint16_t eventLogNextIndex();
bool eventLogWrite(uint8_t type, uint32_t secondsOfDay, uint8_t state, uint8_t profile, uint8_t tri, int16_t waterMm, int16_t pressureDeltaTenthHpa, int16_t tempTenthC, uint8_t humidityPct, uint8_t faults);

#endif
