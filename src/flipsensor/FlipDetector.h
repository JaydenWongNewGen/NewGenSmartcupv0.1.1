#ifndef FLIP_DETECTOR_H
#define FLIP_DETECTOR_H

#include <Arduino.h>
#include <Wire.h>

class FlipDetector {
public:
    FlipDetector(TwoWire& imuWire);

    void begin();
    bool isFlipped();

private:
    TwoWire& wire;
    bool initialized = false;
    float baselineZ = 0.0f;
    unsigned long lastFlipCheck = 0;
};

#endif // FLIP_DETECTOR_H
