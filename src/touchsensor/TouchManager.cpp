#include "TouchManager.h"

TouchManager::TouchManager(CST816S& touchDriver) : touch(touchDriver) {}

static void scanI2CBus(TwoWire& bus) {
    Serial.println("TouchManager: I2C scan start");
    for (uint8_t addr = 1; addr < 0x7F; addr++) {
        bus.beginTransmission(addr);
        if (bus.endTransmission() == 0) {
            Serial.printf("  found device at 0x%02X\n", addr);
        }
    }
    Serial.println("TouchManager: I2C scan done");
}

void TouchManager::begin(TwoWire& bus) {
    touch.begin(bus, FALLING);
    if (touch.detectAddress() && touch.probe()) {
        Serial.println("TouchManager: CST816S detected");
    } else {
        Serial.println("TouchManager: CST816S not responding (check I2C pins/address)");
        scanI2CBus(bus);
    }
}

static String deriveGesture(uint16_t prevX, uint16_t prevY, uint16_t x, uint16_t y, const String& rawGesture) {
    const int threshold = 30;
    int dx = (int)x - (int)prevX;
    int dy = (int)y - (int)prevY;
    if (abs(dx) > abs(dy) && abs(dx) > threshold) {
        return dx > 0 ? "SWIPE RIGHT" : "SWIPE LEFT";
    } else if (abs(dy) > threshold) {
        return dy > 0 ? "SWIPE DOWN" : "SWIPE UP";
    }
    return rawGesture;
}

bool TouchManager::isTouched() {
    static unsigned long lastPollMs = 0;
    bool got = false;

    if (touch.available()) {
        got = true;
    } else {
        unsigned long now = millis();
        if (now - lastPollMs >= 50) {  // throttle polling to reduce I2C spam
            got = touch.poll();
            lastPollMs = now;
        }
    }

    if (got) {
        prevX = lastX;
        prevY = lastY;
        lastX = touch.data.x;
        lastY = touch.data.y;
        lastGestureName = deriveGesture(prevX, prevY, lastX, lastY, touch.gesture());
        return true;
    }
    return false;
}

bool TouchManager::isTouchInRegion(const TouchRegion& region) {
    return isTouched() &&
           lastX >= region.x &&
           lastX <= region.x + region.w &&
           lastY >= region.y &&
           lastY <= region.y + region.h;
}

uint16_t TouchManager::getTouchX() const {
    return lastX;
}

uint16_t TouchManager::getTouchY() const {
    return lastY;
}
