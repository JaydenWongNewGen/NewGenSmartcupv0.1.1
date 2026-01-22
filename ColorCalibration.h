#ifndef COLOR_CALIBRATION_H
#define COLOR_CALIBRATION_H

#define SENSOR_ID 1 // change depending on the sensor you are working with
#define DEBUG_SKIP_TO_ANALYSIS true // change depending on whether you want to skip straight to color sensing

struct ColorCalibration {
  uint16_t blackR, whiteR;
  uint16_t blackG, whiteG;
  uint16_t blackB, whiteB;
  uint16_t blackC, whiteC;
};

// This is the section you change to calibrate the sensor
// To calibrate, place a black square in the assay holder, and change the first number of each row below.
// Then place a white square, and change the second number of each row below.
// Use the *raw* numbers, not the processed ones.
// Color channel correction to follow.
inline const ColorCalibration SENSOR1 = {
  118, 1115,  // R
  180, 1774,  // G
  203, 2061,  // B
  453, 3004 // C - This value (C) is not necessary, and doesn't impact anything. I just use it as a benchmark to monitor drift.
};

inline const ColorCalibration SENSOR2 = {
  97, 396,  // R
  145, 629,  // G
  166,  740,  // B
  949, 8553   // C - This is not necessary, see above.
};

#if SENSOR_ID == 1
inline const ColorCalibration& calib = SENSOR1;
#else
inline const ColorCalibration& calib = SENSOR2;
#endif

#endif  // COLOR_CALIBRATION_H
