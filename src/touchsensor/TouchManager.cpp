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
bool TouchManager::isTouched() {
    if (touch.available() || touch.poll()) {
        lastX = touch.data.x;
        lastY = touch.data.y;
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
