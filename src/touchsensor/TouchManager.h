#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include "sensors.h"  // for CST816S

struct TouchRegion {
    int x, y, w, h;
};

class TouchManager {
public:
    TouchManager(CST816S& touchDriver);

    void begin();
    bool isTouched();
    bool isTouchInRegion(const TouchRegion& region);
    uint16_t getTouchX() const;
    uint16_t getTouchY() const;

private:
    CST816S& touch;
    uint16_t lastX = 0;
    uint16_t lastY = 0;
};

#endif  // TOUCH_MANAGER_H
