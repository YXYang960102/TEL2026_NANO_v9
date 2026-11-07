#pragma once
#include <Arduino.h>

constexpr uint8_t PIN_DR1 = A3;
constexpr uint8_t PIN_DR2 = A2;
constexpr uint8_t PIN_DW1 = A1;

constexpr uint8_t PIN_MA = 11;
constexpr uint8_t PIN_MC = 3;
constexpr uint8_t PIN_MB = 12;
constexpr uint8_t PIN_MD = 7;

constexpr uint8_t PIN_SA = 6;
constexpr uint8_t PIN_SB = 5;
constexpr uint8_t PIN_SC = 4;
constexpr uint8_t PIN_SD = A4;

constexpr uint8_t MA_RUN_PWM = 0;
constexpr uint8_t MA_SLOW_PWM = 100;
constexpr uint8_t MA_STOP_PWM = 255;
constexpr uint8_t MC_RUN_PWM = 0;
constexpr uint8_t MC_STOP_PWM = 255;

constexpr int MB_STOP_US = 1500;
constexpr int MB_RUN_US = 2000;
constexpr int MD_STOP_US = 1500;
constexpr int MD_RUN_US = 1700;

constexpr unsigned long MD_RUN_TIME_MS = 600UL;
constexpr unsigned long MD_STOP_TIME_MS = 1000UL;
constexpr unsigned long ESC_STARTUP_TIME_MS = 3000UL;
constexpr unsigned long INPUT_SAMPLE_INTERVAL_MS = 5UL;
constexpr uint8_t INPUT_CONFIRM_COUNT = 3;
constexpr unsigned long SA_HIGH_CONFIRM_TIME_MS = 500UL;
constexpr unsigned long DR2_STOP_HIGH_TIME_MS = 50UL;
constexpr unsigned long DR2_RELEASE_LOW_TIME_MS = 50UL;
constexpr unsigned long DW1_PULSE_TIME_MS = 100UL;
constexpr unsigned long SERIAL_REPORT_INTERVAL_MS = 250UL;
