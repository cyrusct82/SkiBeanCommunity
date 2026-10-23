#pragma once
#include <Arduino.h>
#include <SPI.h>

// MAX6675 (K-type) on ESP32-S3 DevKitC-1, hardware SPI (FSPI)
// Wiring (same pins as the matelbear/SkywalkerRoasterLab Thermocouple.ino reference):
//   MAX6675 SCK->GPIO13, SO->GPIO11, CS->GPIO10, VCC->3V3, GND->GND
#define ET_SCK_PIN 13
#define ET_SO_PIN  11
#define ET_CS_PIN  10
// Which TC4 channel carries ET in the READ reply: 1 or 2.
// Reference firmware sends roaster probe on channel 1 and MAX6675 on channel 2.
#define ET_CHANNEL 2
#define ET_READ_INTERVAL_MS 300   // MAX6675 conversion takes ~220 ms

class SkiET {
public:
    void begin() {
        pinMode(ET_CS_PIN, OUTPUT);
        digitalWrite(ET_CS_PIN, HIGH);
        _spi.begin(ET_SCK_PIN, ET_SO_PIN, -1, ET_CS_PIN);
    }

    // call from loop(); non-blocking, reads at most every ET_READ_INTERVAL_MS
    void update() {
        uint32_t now = millis();
        if (now - _last < ET_READ_INTERVAL_MS) return;
        _last = now;

        _spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        digitalWrite(ET_CS_PIN, LOW);
        uint16_t raw = _spi.transfer16(0x0000);
        digitalWrite(ET_CS_PIN, HIGH);
        _spi.endTransaction();

        if (raw & 0x0004) {            // bit D2 = thermocouple open
            _ok = false;
            return;
        }
        _ok = true;
        _tempC = (raw >> 3) * 0.25;    // 12-bit, 0.25 C/LSB
    }

    bool   valid()  const { return _ok; }
    double getC()   const { return _tempC; }
    double get(char unit) const { return unit == 'F' ? _tempC * 9.0 / 5.0 + 32.0 : _tempC; }

private:
    SPIClass _spi{FSPI};
    uint32_t _last = 0;
    double   _tempC = 0.0;
    bool     _ok = false;
};
