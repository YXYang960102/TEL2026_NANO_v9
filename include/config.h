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
constexpr int MD_RUN_US = 1600;
// The off part of MD's cycle turns it backward now (Jeremy, 2026-11-08; it
// used to stop, MD_STOP_US, for 600 ms): forward MD_RUN_US for
// MD_RUN_TIME_MS, backward MD_REVERSE_US for MD_REVERSE_TIME_MS, stopped
// (MD_STOP_US) for MD_PAUSE_TIME_MS before forward again.
constexpr int MD_REVERSE_US = 1400;

constexpr unsigned long MD_RUN_TIME_MS = 600UL;
constexpr unsigned long MD_REVERSE_TIME_MS = 300UL;
constexpr unsigned long MD_PAUSE_TIME_MS = 50UL;
constexpr unsigned long ESC_STARTUP_TIME_MS = 3000UL;
constexpr unsigned long INPUT_SAMPLE_INTERVAL_MS = 5UL;
constexpr uint8_t INPUT_CONFIRM_COUNT = 3;
// One fire request = one ball: need_pieces drops as soon as SA sees the ball
// leave (LOW -> HIGH). Was "SA HIGH for 0.5 s", which never happened while
// the next ball was right behind, so one request emptied the tower.
// Safety: MA stops pushing if no ball leaves within this time.
constexpr unsigned long FEED_TIMEOUT_MS = 1500UL;
// After the last requested ball leaves SA, MA keeps full speed this long so
// the ball clears the top into the flywheel. From the fire request to the
// end of this, MB is stopped, so the second ball is not pushed up into the
// first one (they jammed at the top). PROVISIONAL: tune on the robot --
// too short and the top ball still jams, too long and the second ball goes
// out too.
constexpr unsigned long CLEAR_MS = 200UL;
constexpr unsigned long DR2_STOP_HIGH_TIME_MS = 50UL;
constexpr unsigned long DR2_RELEASE_LOW_TIME_MS = 50UL;
constexpr unsigned long DW1_PULSE_TIME_MS = 100UL;
constexpr unsigned long SERIAL_REPORT_INTERVAL_MS = 250UL;
