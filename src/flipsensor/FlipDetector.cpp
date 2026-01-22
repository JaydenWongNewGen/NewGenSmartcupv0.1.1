#include "FlipDetector.h"
#include "sensors.h"  // For QMI8658_read_xyz

FlipDetector::FlipDetector(TwoWire& imuWire) : wire(imuWire) {}

void FlipDetector::begin() {
    // Any initialization needed later
}

bool FlipDetector::isFlipped() {
    float acc[3], gyro[3];
    unsigned int tim;
    QMI8658_read_xyz(acc, gyro, &tim);

    if (!initialized) {
        baselineZ = acc[2];  // assume this is upright
        initialized = true;
        Serial.printf("Baseline Z set: %.2f\n", baselineZ);
        return false;
    }

    // Debounce: check every 500ms
    if (millis() - lastFlipCheck < 500) return false;
    lastFlipCheck = millis();

    float deltaZ = acc[2] - baselineZ;
    Serial.printf("Flip delta Z: %.2f\n", deltaZ);
    Serial.printf("ACC X: %.2f\n", acc[0]);
    Serial.printf("ACC Y: %.2f\n", acc[1]);
    Serial.printf("ACC Z: %.2f\n", acc[2]);

    // Flip threshold
    if (deltaZ > 1600.0f) {
        Serial.println("Cup flipped detected.");
        return true;
    }

    return false;
}
