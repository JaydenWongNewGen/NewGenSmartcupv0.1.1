#ifndef _LCD_TEST_H_
#define _LCD_TEST_H_

#include "DEV_Config.h"
#include "src/screen/GUI_Paint.h"
#include "src/screen/ImageData.h"
#include "src/screen/LCD_1in28.h"
#include "src/flipsensor/QMI8658.h"
#include "src/touchsensor/CST816S.h"
#include <stdlib.h> // malloc() free()

int LCD_1in28_test(void);

#endif
