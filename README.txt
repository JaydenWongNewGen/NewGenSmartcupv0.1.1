SmartCup v0.1
-------------

An ESP32-based embedded system that uses a color sensor, touch input, IMU, and BLE to detect and display the color of an assay.

Note: Touch Sensor still doesn't work.

Compile:
- Arduino IDE
- Requires these libraries:
	- Adafruit_TCS34725 by Adafruit
	- Adafruit BusIO by Adafruit
	- TFT_eSPI by Bodmer
	- lvgl by kisvegabor
- In the Boards Manager, download and install: "esp32" by Espressif Systems
- For the board, select: "Waveshare ESP32-S3-Zero"
- In the top bar, go to 'Tools' -> PSRAM -> Enabled
- Ensure you are plugged into the correct port, and compile!

Config:
- For configuration, you only need to edit `ColorCalibration.h`
	- Select `SENSOR_ID` based on the sensor you are using
	- Change `DEBUG_SKIP_TO_ANALYSIS` depending on your needs.
	- Instructions for black and white level calibration are included in the relevant file.


Key Modules:
- BluetoothManager: Handles BLE GATT connection.
- ColorProcessor: Reads TCS34725 and performs calibration + gamma/saturation correction.
- FlipDetector: Uses QMI8658 IMU to detect cup orientation.
- TouchManager: Handles CST816S touch input and regions.
- UI logic currently handled inline via LCD_Test + Paint.


Setup:
- ESP32 with PSRAM
- TCS34725 Color sensor
- CST816S touch sensor
- QMI8658 IMU (accelerometer, gyroscope,etc.)
- 240x240 LCD (1.28")