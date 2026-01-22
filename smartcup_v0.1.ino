#include <Arduino.h>
#include <Wire.h>
#include <pgmspace.h>
#include "src/images/logos.h"
#include "ColorCalibration.h"
#include "LCD_Test.h"
#include "sensors.h"
#include "src/fonts/fonts.h"
#include "src/bluetooth/BluetoothManager.h"

// TCS34725 LED Wire
#define LED_PIN 15

uint16_t SCREEN_W;
uint16_t SCREEN_H;
uint32_t NumPixels;

// Sensor & Touch
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_360MS, TCS34725_GAIN_1X);
CST816S touch(6, 7, 13, 5);  // SDA, SCL, RST, IRQ
TwoWire IMUWire(1);          // Use bus #1 (Wire1)

// Buffers
UWORD* FrameBuffer = NULL;
UWORD* LogoImage = NULL;
ColorProcessor* colorProcessor = nullptr;

FlipDetector flipDetector(IMUWire);
TouchManager touchManager(touch);
BluetoothManager ble;

// App State
enum AppState {
  STATE_SPLASH,
  STATE_WAIT_FLIP,
  STATE_WAIT_TOUCH,
  STATE_ANALYSIS
};
AppState currentState = STATE_SPLASH;

static unsigned long lastSampleMs = 0;
static const uint32_t SAMPLE_PERIOD_MS = 200; // send 5 Hz; tweak as needed
static const uint8_t BACKLIGHT_BRIGHT = 100;
static const uint8_t BACKLIGHT_DIM = 20;
static const uint32_t BACKLIGHT_IDLE_MS = 300000;

static unsigned long lastInteractionMs = 0;
static bool backlightDimmed = false;

void showLogoWithLoading() {
  drawImageToFrameBuffer(newgensmall);
  const char* msg = "loading";
  int x = (240 - strlen(msg) * 8) / 2;
  Paint_DrawString_EN(x, 200, msg, &Font12, WHITE, BLACK);
  LCD_1IN28_Display(FrameBuffer);
}

void sendBleSample() {
  uint16_t r, g, b, c;

  // light the TCS LED briefly for a stable reading (uses your LED_PIN 15)
  digitalWrite(LED_PIN, HIGH);
  delay(3);
  tcs.getRawData(&r, &g, &b, &c);
  digitalWrite(LED_PIN, LOW);

  // compact JSON under 180 bytes
  char buf[128];
  snprintf(buf, sizeof(buf),
           "{\"r\":%u,\"g\":%u,\"b\":%u,\"c\":%u,\"t\":%lu}",
           r, g, b, c, millis());
  ble.notifyJSON(String(buf));
}

void markInteraction() {
  lastInteractionMs = millis();
  if (backlightDimmed) {
    DEV_SET_PWM(BACKLIGHT_BRIGHT);
    backlightDimmed = false;
  }
}

void updateBacklight() {
  if (!backlightDimmed && millis() - lastInteractionMs >= BACKLIGHT_IDLE_MS) {
    DEV_SET_PWM(BACKLIGHT_DIM);
    backlightDimmed = true;
  }
}

void logTouchIfAny() {
  if (touchManager.isTouched()) {
    markInteraction();
    Serial.printf("Touch: x=%u y=%u gesture=%s\n",
                  touchManager.getTouchX(),
                  touchManager.getTouchY(),
                  touchManager.getGestureName().c_str());
  }
}

void handleUartCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '1') {
      Serial.println("UART: forcing analysis state");
      showLogoWithLoading();
      currentState = STATE_ANALYSIS;
    } else if (c == '2') {
      Serial.printf("UART: last touch x=%u y=%u gesture=%s\n",
                    touchManager.getTouchX(),
                    touchManager.getTouchY(),
                    touchManager.getGestureName().c_str());
    }
  }
}

// Button UI
const int BTN_X = 20;
const int BTN_Y = 100;
const int BTN_W = 200;
const int BTN_H = 50;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");
  delay(200);

  // ✅ Set screen size early
  SCREEN_W = LCD_1IN28_WIDTH;
  SCREEN_H = LCD_1IN28_HEIGHT;
  NumPixels = SCREEN_W * SCREEN_H;

  pinMode(LED_PIN, OUTPUT);

  Wire.begin(21, 33);
  IMUWire.begin(6, 7);
  IMUWire.setClock(100000);
  ble.begin();
  Wire.setClock(100000);

  if (DEV_Module_Init() != 0) Serial.println("GPIO Init Fail!");
  LCD_1IN28_Init(HORIZONTAL);
  DEV_SET_PWM(BACKLIGHT_BRIGHT);
  markInteraction();

  touchManager.begin(IMUWire);

  if (!psramInit()) {
    Serial.println("PSRAM not available, falling back to heap (reduced buffer reliability)");
  }

  // ✅ Allocate framebuffer after knowing NumPixels
  FrameBuffer = (UWORD*)ps_malloc(NumPixels * sizeof(UWORD));
  if (!FrameBuffer) {
    FrameBuffer = (UWORD*)malloc(NumPixels * sizeof(UWORD));
  }
  if (!FrameBuffer) {
    Serial.println("FrameBuffer allocation failed!");
    while (1);
  }

  if (!tcs.begin()) Serial.println("TCS34725 not found!");
  else Serial.println("TCS34725 OK");

  // ✅ Create ColorProcessor after FrameBuffer is ready
  colorProcessor = new ColorProcessor(tcs, FrameBuffer, NumPixels, LED_PIN);
  colorProcessor->begin();

  Serial.println("Touch OK");

  if (!QMI8658_init(IMUWire)) {
    Serial.println("IMU init failed!");
  } else Serial.println("QMI8658 IMU init OK");

  flipDetector.begin();

  Paint_NewImage((UBYTE*)FrameBuffer, SCREEN_W, SCREEN_H, 0, WHITE);
  delay(100);

  Paint_SetScale(65);
  Paint_SetRotate(ROTATE_0);
  Paint_Clear(WHITE);

  // Skip splash and BLE if debug mode is enabled
  if (DEBUG_SKIP_TO_ANALYSIS) {
    Serial.println("DEBUG MODE: Skipping logo, BLE, and flip detection...");
    currentState = STATE_ANALYSIS;
    return;
  }

  drawImageToFrameBuffer(newgenbig);
  LCD_1IN28_Display(FrameBuffer);
  delay(3000);

  drawImageToFrameBuffer(newgensmall);
  LCD_1IN28_Display(FrameBuffer);

  Serial.println("Waiting for BLE client connection...");
  int dotCount = 0;
  while (!ble.isDeviceConnected()) {
    drawImageToFrameBuffer(newgensmall);

    const char* line1 = "Waiting for";
    int line1_x = (240 - strlen(line1) * 8) / 2;
    Paint_DrawString_EN(line1_x, 180, line1, &Font12, WHITE, BLACK);

    char msg[40];
    snprintf(msg, sizeof(msg), "BLE Connection%.*s", dotCount, "...");
    int line2_x = (240 - strlen(msg) * 8) / 2;
    Paint_DrawString_EN(line2_x, 200, msg, &Font12, WHITE, BLACK);

    LCD_1IN28_Display(FrameBuffer);

    dotCount = (dotCount + 1) % 4;
    delay(500);
  }

  Serial.println("BLE client connected. Proceeding with sketch...");
  drawImageToFrameBuffer(newgensmall);
  LCD_1IN28_Display(FrameBuffer);

  currentState = STATE_WAIT_FLIP;
}

void loop() {
  static int dotCount = 0;
  static unsigned long lastAnim = 0;
  handleUartCommands();
  logTouchIfAny();
  switch (currentState) {
    case STATE_WAIT_FLIP:
      if (millis() - lastAnim > 500) {
        lastAnim = millis();
        drawImageToFrameBuffer(newgensmall);

        const char* baseMsg = "Waiting for flip";
        char msg[40];
        snprintf(msg, sizeof(msg), "%s%.*s", baseMsg, dotCount, "...");
        int string_xpos = ((240 - strlen(msg) * 8) / 2) + 5;
        Paint_DrawString_EN(string_xpos, 190, msg, &Font12, WHITE, BLACK);
        LCD_1IN28_Display(FrameBuffer);

        dotCount = (dotCount + 1) % 4;
      }

      if (flipDetector.isFlipped()) {
			showLogoWithLoading();          // <<< only logo + "loading"
			currentState = STATE_ANALYSIS;  // start streaming loop
		}

      break;

	case STATE_ANALYSIS:
	  // Do NOT call colorProcessor->runAnalysis() here anymore.
	  if (ble.isDeviceConnected() && millis() - lastSampleMs >= SAMPLE_PERIOD_MS) {
		lastSampleMs = millis();
		sendBleSample();
	  }
	  break;


    default:
      break;
  }

  updateBacklight();
  delay(100);
}

void drawStartButton() {
  Paint_Clear(WHITE);
  Paint_DrawRectangle(BTN_X, BTN_Y, BTN_X + BTN_W, BTN_Y + BTN_H, 0x229f, DOT_PIXEL_2X2, DRAW_FILL_FULL);
  Paint_DrawString_EN(BTN_X + 10, BTN_Y + 15, "Starting Analysis", &Font16, WHITE, WHITE);
  LCD_1IN28_Display(FrameBuffer);
}

void drawImageToFrameBuffer(const uint16_t* imgData) {
  for (uint32_t i = 0; i < NumPixels; i++) {
    FrameBuffer[i] = pgm_read_word(&imgData[i]);
  }
}
