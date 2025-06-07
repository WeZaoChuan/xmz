#ifndef __USART_APP_H__
#define __USART_APP_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

int my_printf(uint32_t usart_periph, const char *format, ...);
void uart_task(void);
void print_adc_file_info(void); // ��ӡADC�ļ���Ϣ������

#ifdef __cplusplus
}
#endif

#endif
