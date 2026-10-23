#pragma once
#include <Arduino.h>

// 24VDC drum motor via D4184 PWM MOSFET module (Arduino-ESP32 core 3.x / pioarduino)
// Wiring: GPIO4 -> D4184 PWM/trigger IN, GND common with 24V supply negative.
// Motor+ to 24V+, Motor- to D4184 output. Flyback diode across motor (cathode to +24V).
#define DRUM_PWM_PIN    4
#define DRUM_PWM_FREQ   5000   // Hz
#define DRUM_PWM_BITS   10
#define DRUM_MIN_PCT    20     // below this the motor stalls: tune on your machine
#define DRUM_KICK_PCT   60     // short kick to overcome stiction
#define DRUM_KICK_MS    300
#define DRUM_RAMP_STEP_MS 40   // +/-1 % per step

class SkiDrum {
public:
    void begin() {
        ledcAttach(DRUM_PWM_PIN, DRUM_PWM_FREQ, DRUM_PWM_BITS);
        write(0);
    }

    // 0 = off, 1..100 = speed %. Values under DRUM_MIN_PCT are raised to it.
    void setSpeed(int pct) {
        pct = constrain(pct, 0, 100);
        if (pct > 0 && pct < DRUM_MIN_PCT) pct = DRUM_MIN_PCT;
        if (_target == 0 && pct > 0) _kickUntil = millis() + DRUM_KICK_MS;
        _target = pct;
    }
    int  getSpeed() const { return _target; }
    void stop() { _target = 0; _current = 0; _kickUntil = 0; write(0); }

    // call from loop()
    void update() {
        uint32_t now = millis();
        if (_kickUntil && now < _kickUntil) { write(DRUM_KICK_PCT); return; }
        _kickUntil = 0;
        uint32_t elapsed = now - _lastStep;
        if (elapsed < DRUM_RAMP_STEP_MS) return;
        int steps = elapsed / DRUM_RAMP_STEP_MS;   // loop() can be slow, so step by elapsed time
        _lastStep = now;
        if (_current < _target) _current = min(_target, _current + steps);
        else if (_current > _target) _current = max(_target, _current - steps);
        write(_current);
    }

private:
    int _target = 0, _current = 0;
    uint32_t _kickUntil = 0, _lastStep = 0;
    void write(int pct) {
        ledcWrite(DRUM_PWM_PIN, (uint32_t)pct * ((1 << DRUM_PWM_BITS) - 1) / 100);
    }
};
