#include <Arduino.h>
#include <Servo.h>
#include "config.h"

struct DebouncedInput {
    bool stableState = LOW;
    bool candidateState = LOW;
    uint8_t candidateCount = 0;
};

Servo mbEsc;
Servo mdEsc;

DebouncedInput dr1Input, dr2Input, saInput, sbInput, scInput, sdInput;

long need_pieces = 0;
unsigned long startupTime = 0;
unsigned long lastSampleTime = 0;
unsigned long lastReportTime = 0;

unsigned long feedStartTime = 0;
bool feedClearing = false;
unsigned long clearStartTime = 0;

bool systemDisabled = false;
bool dr2HighTiming = false;
bool dr2LowTiming = false;
unsigned long dr2HighStartTime = 0;
unsigned long dr2LowStartTime = 0;

// MD's cycle: forward, backward, a short stop, forward again.
enum MdPhase : uint8_t { MD_FORWARD, MD_REVERSE, MD_PAUSE };
MdPhase mdPhase = MD_FORWARD;
unsigned long mdPhaseStartTime = 0;

uint8_t maOutput = MA_STOP_PWM;
uint8_t mcOutput = MC_STOP_PWM;
int mbOutputUs = MB_STOP_US;
int mdOutputUs = MD_STOP_US;
bool dw1Output = LOW;
bool dw1PulseActive = false;
unsigned long dw1PulseStartTime = 0;
bool dw1BallWasPresent = false;
bool dw1WasEnabled = false;
bool dw1WasFiring = false;

bool readInput(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}

void initializeInput(DebouncedInput &input, bool initialState) {
    input.stableState = initialState;
    input.candidateState = initialState;
    input.candidateCount = 0;
}

bool updateInput(DebouncedInput &input, bool rawState) {
    if (rawState == input.stableState) {
        input.candidateState = rawState;
        input.candidateCount = 0;
        return false;
    }

    if (rawState != input.candidateState) {
        input.candidateState = rawState;
        input.candidateCount = 1;
        return false;
    }

    if (input.candidateCount < 255) input.candidateCount++;

    if (input.candidateCount >= INPUT_CONFIRM_COUNT) {
        input.stableState = input.candidateState;
        input.candidateCount = 0;
        return true;
    }
    return false;
}

void processDr1Event(bool previousState, unsigned long now) {
    if (systemDisabled) return;

    if (previousState == LOW && dr1Input.stableState == HIGH) {
        if (need_pieces > 0) {
            // The last requested ball has not left SA yet: it is stuck. Do
            // not queue a second one (2026-11-14, Jeremy: two balls went out
            // on one press once the jam cleared); FEED_TIMEOUT_MS still
            // counts from the first request.
            Serial.println(F("[EVENT] DR1 while ball still at SA: jam, not added"));
        } else if (saInput.stableState == LOW || sbInput.stableState == LOW) {
            feedStartTime = now;
            need_pieces = 1;
            Serial.print(F("[EVENT] DR1 ADD, need_pieces="));
            Serial.println(need_pieces);
        } else {
            Serial.println(F("[EVENT] DR1 ignored: SA/SB both HIGH"));
        }
    }
}

// A ball left SA (LOW -> HIGH): that request is done, even if the next ball
// is already right behind it.
void processSaStableChange(bool previousState, unsigned long now) {
    if (previousState == LOW && saInput.stableState == HIGH && need_pieces > 0) {
        need_pieces--;
        feedStartTime = now;
        if (need_pieces == 0) {
            feedClearing = true; // MA on for CLEAR_MS more, MB still stopped
            clearStartTime = now;
        }
        Serial.print(F("[EVENT] SA ball left, need_pieces="));
        Serial.println(need_pieces);
    }
}

void updateFeedTimeout(unsigned long now) {
    if (need_pieces > 0 && now - feedStartTime >= FEED_TIMEOUT_MS) {
        need_pieces = 0;
        feedClearing = false; // stuck: stop pushing, no clear time
        Serial.println(F("[EVENT] feed timeout, need_pieces=0"));
    }
    if (feedClearing && now - clearStartTime >= CLEAR_MS) {
        feedClearing = false;
        Serial.println(F("[EVENT] clear done"));
    }
}

void updateDr2DisableLogic(unsigned long now) {
    if (!systemDisabled) {
        if (dr2Input.stableState == HIGH) {
            if (!dr2HighTiming) {
                dr2HighTiming = true;
                dr2HighStartTime = now;
            }
            if (now - dr2HighStartTime >= DR2_STOP_HIGH_TIME_MS) {
                systemDisabled = true;
                need_pieces = 0;
                feedClearing = false;
                dr2HighTiming = false;
                dr2LowTiming = false;
                Serial.println(F("[DISABLE] DR2 HIGH > 50ms"));
            }
        } else {
            dr2HighTiming = false;
        }
    } else {
        if (dr2Input.stableState == LOW) {
            if (!dr2LowTiming) {
                dr2LowTiming = true;
                dr2LowStartTime = now;
            }
            if (now - dr2LowStartTime >= DR2_RELEASE_LOW_TIME_MS) {
                systemDisabled = false;
                dr2LowTiming = false;
                dr2HighTiming = false;
                Serial.println(F("[ENABLE] DR2 LOW > 50ms"));
            }
        } else {
            dr2LowTiming = false;
        }
    }
}

void sampleInputs(unsigned long now) {
    bool previousState = saInput.stableState;
    if (updateInput(saInput, readInput(PIN_SA))) {
        processSaStableChange(previousState, now);
    }

    updateInput(sbInput, readInput(PIN_SB));
    updateInput(scInput, readInput(PIN_SC));
    updateInput(sdInput, readInput(PIN_SD));
    updateInput(dr2Input, readInput(PIN_DR2));

    previousState = dr1Input.stableState;
    if (updateInput(dr1Input, readInput(PIN_DR1))) {
        processDr1Event(previousState, now);
    }

    updateFeedTimeout(now);
}

// DW1 -> Mega A14: one 100 ms pulse per ball (Mega counts rising edges).
// Pulse when a ball arrives at SA/SB while enabled, or on enable with a ball already there.
void updateDw1(unsigned long now) {
    const bool ballPresent = (saInput.stableState == LOW) || (sbInput.stableState == LOW);
    const bool enabled = !systemDisabled;

    // A fire just finished (feed + clear time, or the feed timeout) with a
    // ball still at the top: pulse again (2026-11-14). The next ball can sit
    // right behind the one that left, so SA/SB may never read empty and the
    // arrival pulse above would not come; the Mega cleared its "ball ready"
    // at the fire and would wait for good. An extra pulse while the Mega
    // already has one latched is not counted twice there.
    const bool firing = need_pieces > 0 || feedClearing;
    const bool fireEnded = dw1WasFiring && !firing;
    if (enabled && ballPresent && (!dw1BallWasPresent || !dw1WasEnabled || fireEnded) && !dw1PulseActive) {
        dw1PulseActive = true;
        dw1PulseStartTime = now;
        Serial.println(fireEnded ? F("[EVENT] DW1 ready pulse (after fire)") : F("[EVENT] DW1 ready pulse"));
    }
    dw1BallWasPresent = ballPresent;
    dw1WasEnabled = enabled;
    dw1WasFiring = firing;

    if (dw1PulseActive && now - dw1PulseStartTime >= DW1_PULSE_TIME_MS) {
        dw1PulseActive = false;
    }

    dw1Output = dw1PulseActive;
    digitalWrite(PIN_DW1, dw1Output ? HIGH : LOW);
}

void updateMdOutput(unsigned long now, bool escReady) {
    if (!escReady || systemDisabled) {
        mdPhase = MD_FORWARD;
        mdPhaseStartTime = now;
        mdOutputUs = MD_STOP_US;
        mdEsc.writeMicroseconds(mdOutputUs);
        return;
    }

    if (sdInput.stableState == LOW) {
        mdPhase = MD_FORWARD;
        mdPhaseStartTime = now;
        mdOutputUs = MD_STOP_US;
        mdEsc.writeMicroseconds(mdOutputUs);
        return;
    }

    if (mdPhase == MD_FORWARD) {
        mdOutputUs = MD_RUN_US;
        if (now - mdPhaseStartTime >= MD_RUN_TIME_MS) {
            mdPhase = MD_REVERSE;
            mdPhaseStartTime = now;
        }
    } else if (mdPhase == MD_REVERSE) {
        mdOutputUs = MD_REVERSE_US; // backward, not stopped (2026-11-08)
        if (now - mdPhaseStartTime >= MD_REVERSE_TIME_MS) {
            mdPhase = MD_PAUSE;
            mdPhaseStartTime = now;
        }
    } else {
        mdOutputUs = MD_STOP_US; // stopped between backward and forward
        if (now - mdPhaseStartTime >= MD_PAUSE_TIME_MS) {
            mdPhase = MD_FORWARD;
            mdPhaseStartTime = now;
        }
    }
    mdEsc.writeMicroseconds(mdOutputUs);
}

void stopAllMotors(unsigned long now) {
    maOutput = MA_STOP_PWM;
    mcOutput = MC_STOP_PWM;
    mbOutputUs = MB_STOP_US;
    mdOutputUs = MD_STOP_US;

    analogWrite(PIN_MA, maOutput);
    analogWrite(PIN_MC, mcOutput);
    mbEsc.writeMicroseconds(mbOutputUs);
    mdEsc.writeMicroseconds(mdOutputUs);

    mdPhase = MD_FORWARD;
    mdPhaseStartTime = now;
}

void updateMotorOutputs(unsigned long now) {
    const bool escReady = (now - startupTime) >= ESC_STARTUP_TIME_MS;

    if (!escReady || systemDisabled) {
        stopAllMotors(now);
        return;
    }

    // Firing: from the request until CLEAR_MS after the ball left SA.
    const bool firing = need_pieces > 0 || feedClearing;

    if (firing) {
        maOutput = MA_RUN_PWM;
    } else if (saInput.stableState == HIGH && sbInput.stableState == HIGH) {
        maOutput = MA_SLOW_PWM;
    } else {
        maOutput = MA_STOP_PWM;
    }

    if (firing) {
        mbOutputUs = MB_STOP_US; // don't push the next ball into the one leaving
    } else if ((saInput.stableState == HIGH && sbInput.stableState == HIGH) ||
               scInput.stableState == HIGH) {
        mbOutputUs = MB_RUN_US;
    } else {
        mbOutputUs = MB_STOP_US;
    }

    mcOutput = (sdInput.stableState == HIGH) ? MC_RUN_PWM : MC_STOP_PWM;

    analogWrite(PIN_MA, maOutput);
    analogWrite(PIN_MC, mcOutput);
    mbEsc.writeMicroseconds(mbOutputUs);
    updateMdOutput(now, true);
}

void printState(unsigned long now) {
    const bool escReady = (now - startupTime) >= ESC_STARTUP_TIME_MS;

    Serial.print(F("need=")); Serial.print(need_pieces);
    Serial.print(F(" | DR1=")); Serial.print(dr1Input.stableState ? F("H") : F("L"));
    Serial.print(F(" DR2=")); Serial.print(dr2Input.stableState ? F("H") : F("L"));
    Serial.print(F(" | SA=")); Serial.print(saInput.stableState ? F("H") : F("L"));
    Serial.print(F(" SB=")); Serial.print(sbInput.stableState ? F("H") : F("L"));
    Serial.print(F(" SC=")); Serial.print(scInput.stableState ? F("H") : F("L"));
    Serial.print(F(" SD=")); Serial.print(sdInput.stableState ? F("H") : F("L"));
    Serial.print(F(" | DW1=")); Serial.print(dw1Output ? F("H") : F("L"));
    Serial.print(F(" | MA=")); Serial.print(maOutput);
    Serial.print(F(" MB=")); Serial.print(mbOutputUs); Serial.print(F("us"));
    Serial.print(F(" MC=")); Serial.print(mcOutput);
    Serial.print(F(" MD=")); Serial.print(mdOutputUs); Serial.print(F("us"));
    Serial.print(F(" | ESC=")); Serial.print(escReady ? F("READY") : F("INIT"));
    Serial.print(F(" DISABLE=")); Serial.println(systemDisabled ? F("YES") : F("NO"));
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_MA, OUTPUT);
    pinMode(PIN_MC, OUTPUT);
    pinMode(PIN_DW1, OUTPUT);

    pinMode(PIN_DR1, INPUT);
    pinMode(PIN_DR2, INPUT);
    pinMode(PIN_SA, INPUT);
    pinMode(PIN_SB, INPUT);
    pinMode(PIN_SC, INPUT);
    pinMode(PIN_SD, INPUT);

    mbEsc.attach(PIN_MB);
    mdEsc.attach(PIN_MD);

    analogWrite(PIN_MA, MA_STOP_PWM);
    analogWrite(PIN_MC, MC_STOP_PWM);
    digitalWrite(PIN_DW1, LOW);
    mbEsc.writeMicroseconds(MB_STOP_US);
    mdEsc.writeMicroseconds(MD_STOP_US);

    initializeInput(dr1Input, readInput(PIN_DR1));
    initializeInput(dr2Input, readInput(PIN_DR2));
    initializeInput(saInput, readInput(PIN_SA));
    initializeInput(sbInput, readInput(PIN_SB));
    initializeInput(scInput, readInput(PIN_SC));
    initializeInput(sdInput, readInput(PIN_SD));

    systemDisabled = (dr2Input.stableState == HIGH);

    startupTime = millis();
    lastSampleTime = startupTime;
    lastReportTime = startupTime;
    mdPhaseStartTime = startupTime;

    Serial.println(F("TEL2026 NANO Controller v9"));
}

void loop() {
    const unsigned long now = millis();

    if (now - lastSampleTime >= INPUT_SAMPLE_INTERVAL_MS) {
        lastSampleTime = now;
        sampleInputs(now);
    }

    updateDr2DisableLogic(now);
    updateDw1(now);
    updateMotorOutputs(now);

    if (now - lastReportTime >= SERIAL_REPORT_INTERVAL_MS) {
        lastReportTime = now;
        printState(now);
    }
}
