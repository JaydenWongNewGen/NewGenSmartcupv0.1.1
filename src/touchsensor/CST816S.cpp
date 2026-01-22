/*
   MIT License

  Copyright (c) 2021 Felix Biego

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "Arduino.h"
#include <Wire.h>
#include <FunctionalInterrupt.h>

#include "CST816S.h"


/*!
    @brief  Constructor for CST816S
	@param	sda
			i2c data pin
	@param	scl
			i2c clock pin
	@param	rst
			touch reset pin
	@param	irq
			touch interrupt pin
*/
CST816S::CST816S(int sda, int scl, int rst, int irq) {
  _sda = sda;
  _scl = scl;
  _rst = rst;
  _irq = irq;
  _addr = CST816S_ADDRESS;
  _event_available = false;
}

/*!
    @brief  read touch data
*/
void CST816S::read_touch() {
  byte data_raw[8];
  delay(10);  // small delay to give chip time to wake
  i2c_read(CST816S_ADDRESS, 0x02, data_raw, 6);

  data.gestureID = data_raw[0];
  data.points = data_raw[1];
  data.event = data_raw[2] >> 6;
  data.x = ((data_raw[2] & 0xF) << 8) + data_raw[3];
  data.y = ((data_raw[4] & 0xF) << 8) + data_raw[5];
}

/*!
    @brief  handle interrupts
*/
void IRAM_ATTR CST816S::handleISR(void) {
  _event_available = true;

}

/*!
    @brief  initialize the touch screen
	@param	interrupt
			type of interrupt FALLING, RISING..
*/
void CST816S::begin(TwoWire &wire, int interrupt) {
  _wire = &wire;

  pinMode(_irq, INPUT_PULLUP);
  pinMode(_rst, OUTPUT);

  digitalWrite(_rst, HIGH );
  delay(50);
  digitalWrite(_rst, LOW);
  delay(5);
  digitalWrite(_rst, HIGH );
  delay(50);

  detectAddress();
  i2c_read(_addr, 0x15, &data.version, 1);
  delay(5);
  i2c_read(_addr, 0xA7, data.versionInfo, 3);

  attachInterrupt(_irq, std::bind(&CST816S::handleISR, this), interrupt);
}

/*!
    @brief  check for a touch event
*/
bool CST816S::available() {
  if (_event_available) {
    read_touch();
    _event_available = false;
    return true;
  }
  return false;
}

bool CST816S::poll() {
  byte data_raw[8] = {0};
  static uint8_t failStreak = 0;
  if (i2c_read(_addr, 0x02, data_raw, 6) != 0) {
    failStreak = min<uint8_t>(failStreak + 1, 10);
    return false;
  }
  failStreak = 0;
  // points > 0 indicates at least one touch point present
  if (data_raw[1] == 0) {
    return false;
  }
  data.gestureID = data_raw[0];
  data.points = data_raw[1];
  data.event = data_raw[2] >> 6;
  data.x = ((data_raw[2] & 0xF) << 8) + data_raw[3];
  data.y = ((data_raw[4] & 0xF) << 8) + data_raw[5];
  return true;
}

bool CST816S::probe() {
  uint8_t version = 0;
  return i2c_read(_addr, 0x15, &version, 1) == 0;
}

bool CST816S::detectAddress() {
  const uint8_t candidates[] = {0x15, 0x2E, 0x5A};
  uint8_t version = 0;
  for (uint8_t addr : candidates) {
    if (i2c_read(addr, 0x15, &version, 1) == 0) {
      _addr = addr;
      return true;
    }
  }
  // fallback: leave as default but report failure
  return false;
}

/*!
    @brief  put the touch screen in standby mode
*/
void CST816S::sleep() {
  digitalWrite(_rst, LOW);
  delay(5);
  digitalWrite(_rst, HIGH );
  delay(50);
  byte standby_value = 0x03;
  i2c_write(CST816S_ADDRESS, 0xA5, &standby_value, 1);
}

/*!
    @brief  get the gesture event name
*/
String CST816S::gesture() {
  switch (data.gestureID) {
    case NONE:
      return "NONE";
      break;
    case SWIPE_DOWN:
      return "SWIPE DOWN";
      break;
    case SWIPE_UP:
      return "SWIPE UP";
      break;
    case SWIPE_LEFT:
      return "SWIPE LEFT";
      break;
    case SWIPE_RIGHT:
      return "SWIPE RIGHT";
      break;
    case SINGLE_CLICK:
      return "SINGLE CLICK";
      break;
    case DOUBLE_CLICK:
      return "DOUBLE CLICK";
      break;
    case LONG_PRESS:
      return "LONG PRESS";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}

/*!
    @brief  read data from i2c
	@param	addr
			i2c device address
	@param	reg_addr
			device register address
	@param	reg_data
			array to copy the read data
	@param	length
			length of data
*/
uint8_t CST816S::i2c_read(uint16_t addr, uint8_t reg_addr, uint8_t *reg_data, uint32_t length)
{
  for (int attempt = 0; attempt < 3; attempt++) {
    _wire->beginTransmission(addr);
    _wire->write(reg_addr);
    if (_wire->endTransmission(false) != 0) { // repeated start
      delay(5);
      continue;
    }

    if (_wire->requestFrom((uint8_t)addr, (uint8_t)length, (uint8_t)true) != length) {
      delay(5);
      continue;
    }

    for (int i = 0; i < length; i++) {
      reg_data[i] = _wire->read();
    }
    return 0; // success
  }
  return -1; // fail after retries
}


/*!
    @brief  write data to i2c
	@brief  read data from i2c
	@param	addr
			i2c device address
	@param	reg_addr
			device register address
	@param	reg_data
			data to be sent
	@param	length
			length of data
*/
uint8_t CST816S::i2c_write(uint8_t addr, uint8_t reg_addr, const uint8_t *reg_data, uint32_t length)
{
  _wire->beginTransmission(addr);
  _wire->write(reg_addr);
  for (int i = 0; i < length; i++) {
    _wire->write(*reg_data++);
  }
  if ( _wire->endTransmission(true))return -1;
  return 0;
}
