#ifndef __LED_APP_H__
#define __LED_APP_H__

#include "mydefine.h"

// LED blinking control structure
typedef struct {
    uint8_t led_index;      // LED index (0-5)
    uint16_t on_time;       // LED on time in milliseconds
    uint16_t off_time;      // LED off time in milliseconds
    uint8_t is_blinking;    // Blinking status flag
    uint32_t last_toggle;   // Last toggle timestamp
} LED_Blink_TypeDef;

void led_task(void);
void led_blink_start(uint8_t led_index, uint16_t on_time, uint16_t off_time);
void led_blink_stop(uint8_t led_index);
void led_blink_process(void);

#endif


