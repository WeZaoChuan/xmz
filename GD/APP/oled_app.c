/* Licence
* Company: MCUSTUDIO
* Auther: Ahypnis.
* Version: V0.10
* Time: 2025/06/05
* Note:
*/

#include "mcu_cmic_gd32f470vet6.h"

extern uint16_t adc_value[1];

/**
 * @brief	ʹ������printf�ķ�ʽ��ʾ�ַ�������ʾ6x8��С��ASCII�ַ�
 * @param x  Character position on the X-axis  range��0 - 127
 * @param y  Character position on the Y-axis  range��0 - 3
 * ���磺oled_printf(0, 0, "Data = %d", dat);
 **/
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512]; // ��ʱ�洢��ʽ������ַ���
  va_list arg;      // ����ɱ����
  int len;          // �����ַ�������

  va_start(arg, format);
  // ��ȫ�ظ�ʽ���ַ����� buffer
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

void oled_task(void)
{
    oled_printf(0, 0, "MCUSTUDIO_GD32");
    oled_printf(0, 1, "KEY STA: %d%d%d%d%d%d", KEY1_READ, KEY2_READ, KEY3_READ, KEY4_READ, KEY5_READ, KEY6_READ);
    oled_printf(0, 2, "uwTick:%ld", get_system_ms());
    oled_printf(0, 3, "ADC: %d  ", adc_value[0]);
}

/* CUSTOM EDIT */
