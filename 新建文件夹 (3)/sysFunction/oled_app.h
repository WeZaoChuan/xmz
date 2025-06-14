#ifndef __OLED_APP_H__
#define __OLED_APP_H__

#include "mydefine.h"

int oled_printf(uint8_t x, uint8_t y, const char *format, ...);
void oled_task(void);
 
void OLED_SendBuff(uint8_t buff[4][128]);
void PIDMenu_Init(void);
#endif


