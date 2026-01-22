#include "TouchManager.h"

TouchManager::TouchManager(CST816S& touchDriver) : touch(touchDriver) {}

void TouchManager::begin() {
    touch.begin(Wire, FALLING);
    Serial.println("TouchManager initialized");
}

bool TouchManager::isTouched() {
    if (touch.available()) {
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
