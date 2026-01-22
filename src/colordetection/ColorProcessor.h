#ifndef COLOR_PROCESSOR_H
#define COLOR_PROCESSOR_H

#include <Adafruit_TCS34725.h>

class ColorProcessor {
public:
    ColorProcessor(Adafruit_TCS34725& sensor, uint16_t* frameBuffer, uint32_t numPixels, uint8_t ledPin);
    void begin();
    void runAnalysis();

private:
    Adafruit_TCS34725& tcs;
    uint16_t* FrameBuffer;
    uint32_t NumPixels;
    uint8_t LEDPin;

    float normalize(uint16_t val, uint16_t black, uint16_t white);
    uint8_t gammaCorrect(float val);
    void boostSaturation(uint8_t& r, uint8_t& g, uint8_t& b, float satBoost = 1.5);
    void drawImageToFrameBuffer(const uint16_t* imgData);
};

#endif // COLOR_PROCESSOR_H
