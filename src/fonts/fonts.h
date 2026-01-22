/*
#include "fonts/font8.h" // Ensures Font8 is compiled
#include "fonts/font12.h" // Ensures Font12 is compiled
#include "fonts/font16.h" // Ensures Font16 is compiled
#include "fonts/font20.h" // Ensures Font20 is compiled
#include "fonts/font24.h" // Ensures Font24 is compiled
*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FONTS_H
#define __FONTS_H

// Max font size: Microsoft YaHei 24pt (32x41 pixels)
#define MAX_HEIGHT_FONT         41
#define MAX_WIDTH_FONT          32

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

//ASCII
typedef struct _tFont
{    
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
  
} sFONT;

// External font declarations
extern sFONT Font8;
extern sFONT Font12;
extern sFONT Font16;
extern sFONT Font20;
extern sFONT Font24;

#ifdef __cplusplus
}
#endif
  
#endif /* __FONTS_H */