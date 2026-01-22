#include "ColorProcessor.h"
#include <Arduino.h>
#include <math.h>
#include "LCD_Test.h"
#include "ColorCalibration.h"
#include <pgmspace.h>

ColorProcessor::ColorProcessor(
  Adafruit_TCS34725& tcsSensor,
  uint16_t* framebuffer,
  uint32_t numPixels,
  uint8_t ledPin
) : tcs(tcsSensor), FrameBuffer(framebuffer), NumPixels(numPixels), LEDPin(ledPin) {}


unsigned long lastSensorResetTime = 0;
const unsigned long SENSOR_RESET_INTERVAL = 20000; // 30 seconds


void ColorProcessor::begin() {
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, LOW);

  // Configure TCS34725 interrupt pin (GPIO 18)
  pinMode(18, INPUT_PULLUP);

  // Setup interrupt to trigger on any reading
  tcs.setInterrupt(true);     // Enable hardware interrupt
  tcs.clearInterrupt();       // Clear pending
  tcs.setIntLimits(0, 65535); // Trigger on any ADC value
}

void ColorProcessor::runAnalysis() {
  delay(400);
  digitalWrite(LEDPin, HIGH);       // Turn on LED
  delay(500);                        // Let LED stabilize

  // Check if it's time to reset the sensor
  unsigned long currentTime = millis();
  if (currentTime - lastSensorResetTime > SENSOR_RESET_INTERVAL) {
    Serial.println("Resetting TCS34725 sensor...");
    tcs.disable();   // Power down
    delay(100);      // Allow full shutdown
    tcs.enable();    // Power up
    delay(700);      // Wait for stability
    lastSensorResetTime = currentTime;
  }

  tcs.clearInterrupt();             // Reset interrupt flag
  tcs.setInterrupt(true);           // Enable interrupt output

  // Wait for interrupt signal (GPIO 18 goes LOW)
  unsigned long startTime = millis();
  while (digitalRead(18) == HIGH) {
    if (millis() - startTime > 1000) {
      Serial.println("Timeout waiting for color sensor interrupt.");
      digitalWrite(LEDPin, LOW);
      return;
    }
  }

  // Read color data
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  // Optional: disable interrupt after reading
  tcs.setInterrupt(false);
  delay(400);
  digitalWrite(LEDPin, LOW);        // Turn off LED

  // Debug output
  Serial.printf("R=%u G=%u B=%u C=%u\n", r, g, b, c);

  // === Processing ===
  float normR = normalize(r, calib.blackR, calib.whiteR);
  float normG = normalize(g, calib.blackG, calib.whiteG);
  float normB = normalize(b, calib.blackB, calib.whiteB);
  float normC = normalize(c, calib.blackC, calib.whiteC);

  uint8_t r255 = gammaCorrect(normR);
  uint8_t g255 = gammaCorrect(normG);
  uint8_t b255 = gammaCorrect(normB);

  boostSaturation(r255, g255, b255, 1.5);

  Paint_Clear(WHITE);
  char buf[32];
  bool showCorrectedColor = true;  // Set to true if you want gamma/saturation corrected color

  if (showCorrectedColor) {
    sprintf(buf, "R: %3d = %4d raw", r255, r);
    Paint_DrawString_EN(20, 48, buf, &Font16, RED, WHITE);
    sprintf(buf, "G: %3d = %4d raw", g255, g);
    Paint_DrawString_EN(20, 73, buf, &Font16, GREEN, WHITE);
    sprintf(buf, "B: %3d = %4d raw", b255, b);
    Paint_DrawString_EN(20, 98, buf, &Font16, BLUE, WHITE);
    sprintf(buf, "C: %5u", c);
    Paint_DrawString_EN(20, 123, buf, &Font16, BLACK, WHITE);
  } else {
    uint8_t r8 = normR * 255;
    uint8_t g8 = normG * 255;
    uint8_t b8 = normB * 255;

    sprintf(buf, "R: %3d = %4d raw", r8, r);
    Paint_DrawString_EN(20, 48, buf, &Font16, RED, WHITE);
    sprintf(buf, "G: %3d = %4d raw", g8, g);
    Paint_DrawString_EN(20, 73, buf, &Font16, GREEN, WHITE);
    sprintf(buf, "B: %3d = %4d raw", b8, b);
    Paint_DrawString_EN(20, 98, buf, &Font16, BLUE, WHITE);
    sprintf(buf, "C: %5u", c);
    Paint_DrawString_EN(20, 123, buf, &Font16, BLACK, WHITE);
  }

  // Convert RGB888 to RGB565
  uint16_t color565 = ((r255 & 0xF8) << 8) | ((g255 & 0xFC) << 3) | (b255 >> 3);
  Paint_DrawString_EN(20, 153, "Detected Color:", &Font16, BLACK, WHITE);
  Paint_DrawRectangle(0, 180, 240, 240, color565, DOT_PIXEL_1X1, DRAW_FILL_FULL);

  // Push to screen
  LCD_1IN28_Display(FrameBuffer);
}

float ColorProcessor::normalize(uint16_t val, uint16_t black, uint16_t white) {
  if (val <= black) return 0.0;
  if (val >= white) return 1.0;
  return (float)(val - black) / (white - black);
}

uint8_t ColorProcessor::gammaCorrect(float val) {
  val = constrain(val, 0.0, 1.0);
  return pow(val, 0.5) * 255;
}

void ColorProcessor::boostSaturation(uint8_t& r, uint8_t& g, uint8_t& b, float satBoost) {
  float fr = r / 255.0, fg = g / 255.0, fb = b / 255.0;
  float maxVal = max(fr, max(fg, fb));
  float minVal = min(fr, min(fg, fb));
  float delta = maxVal - minVal;

  float h, s, v = maxVal;
  if (delta < 0.0001) {
    h = 0;
    s = 0;
  } else {
    s = delta / maxVal;
    if (fr == maxVal)
      h = (fg - fb) / delta;
    else if (fg == maxVal)
      h = 2 + (fb - fr) / delta;
    else
      h = 4 + (fr - fg) / delta;

    h *= 60;
    if (h < 0) h += 360;
  }

  s *= satBoost;
  if (s > 1.0) s = 1.0;

  int i = int(h / 60.0) % 6;
  float f = (h / 60.0) - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);

  float rOut, gOut, bOut;
  switch (i) {
    case 0: rOut = v; gOut = t; bOut = p; break;
    case 1: rOut = q; gOut = v; bOut = p; break;
    case 2: rOut = p; gOut = v; bOut = t; break;
    case 3: rOut = p; gOut = q; bOut = v; break;
    case 4: rOut = t; gOut = p; bOut = v; break;
    case 5: rOut = v; gOut = p; bOut = q; break;
  }

  r = constrain(rOut * 255, 0, 255);
  g = constrain(gOut * 255, 0, 255);
  b = constrain(bOut * 255, 0, 255);
}
