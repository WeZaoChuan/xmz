#include "oled_app.h"
 
// �޸�Ϊextern����Ϊ״̬����������btn_app.c��
extern volatile uint8_t oled_display_state; // 0:WuZhaowei:19, 1:Zhoujing, 2:3638, 3:1 5 1
extern volatile uint8_t oled_update_flag;   // ���±�־
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

// u8g2 �� GPIO ����ʱ�ص�����
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch (msg)
  {
  case U8X8_MSG_GPIO_AND_DELAY_INIT:
    // ��ʼ�� GPIO (�����Ҫ������ SPI �� CS, DC, RST ����)
    // ����Ӳ�� I2C������ͨ������Ҫ��ʲô
    break;
  case U8X8_MSG_DELAY_MILLI:
    // ԭ��: u8g2 �ڲ�ĳЩ������Ҫ���뼶����ʱ�ȴ���
    // �ṩ���뼶��ʱ��ֱ�ӵ��� HAL �⺯����
    HAL_Delay(arg_int);
    break;
  case U8X8_MSG_DELAY_10MICRO:
    // ʵ��10΢����ʱ��ʹ�þ�ȷУ׼�Ŀ�ѭ��
    {
      // GD32ϵ��ͨ�������ٶ�Ϊ120-200MHz��ÿ��ѭ����Լ��Ҫ3-4��ʱ������
      // ��160MHz���㣬10��s��ҪԼ400-500��ѭ��
      for (volatile uint32_t i = 0; i < 480; i++)
      {
        __NOP(); // �����������Ż������ָ��
      }
    }
    break;
  case U8X8_MSG_DELAY_100NANO:
    // ʵ��100������ʱ��ʹ�ö��NOPָ��
    // ÿ��NOPָ���Լ��Ҫ1��ʱ������(Լ6ns@160MHz)
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    break;
  case U8X8_MSG_GPIO_I2C_CLOCK: // [[fallthrough]] // Fallthrough ע�ͱ�ʾ����Ϊ֮
  case U8X8_MSG_GPIO_I2C_DATA:
    // ���� SCL/SDA ���ŵ�ƽ����Щ����**���ģ�� I2C** ʱ��Ҫʵ�֡�
    // ʹ��Ӳ�� I2C ʱ����Щ��Ϣ���Ժ��ԣ��� HAL �⴦��
    break;
  // --- ������ GPIO ��ص���Ϣ����Ҫ���ڰ�������� SPI ���� ---
  // ������ u8g2 Ӧ����Ҫ��ȡ��������� SPI ���� (CS, DC, Reset)��
  // ����Ҫ��������� msg ���Ͷ�ȡ/���ö�Ӧ�� GPIO ����״̬��
  // ���ڽ�ʹ��Ӳ�� I2C ��ʾ�ĳ��������������������򵥷��ز�֧�֡�
  case U8X8_MSG_GPIO_CS:
    // SPI Ƭѡ����
    break;
  case U8X8_MSG_GPIO_DC:
    // SPI ����/�����߿���
    break;
  case U8X8_MSG_GPIO_RESET:
    // ��ʾ����λ���ſ���
    break;
  case U8X8_MSG_GPIO_MENU_SELECT:
    u8x8_SetGPIOResult(u8x8, /* ��ȡѡ��� GPIO ״̬ */ 0);
    break;
  default:
    u8x8_SetGPIOResult(u8x8, 1); // ��֧�ֵ���Ϣ
    break;
  }
  return 1;
}

// u8g2 ��Ӳ�� I2C ͨ�Żص�����
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t buffer[32]; // u8g2 ÿ�δ������ 32 �ֽ�
  static uint8_t buf_idx;
  uint8_t *data;

  switch (msg)
  {
  case U8X8_MSG_BYTE_SEND:
    // ԭ��: u8g2 ͨ������һ���Է��ʹ������ݣ����Ƿֿ鷢�͡�
    // �����Ϣ���ڽ�һС������ (arg_int �ֽ�) �� u8g2 �ڲ����ݵ����ǵĻص�������
    // ������Ҫ����Щ�����ݴ浽���� buffer �У��ȴ� START/END_TRANSFER �źš�
    data = (uint8_t *)arg_ptr;
    while (arg_int > 0)
    {
      buffer[buf_idx++] = *data;
      data++;
      arg_int--;
    }
    break;
  case U8X8_MSG_BYTE_INIT:
    // ԭ��: �ṩһ��������� I2C ����ĳ�ʼ����
    // ��ʼ�� I2C (ͨ���� main �����������)
    // ���������� main �������Ѿ������� MX_I2C1_Init()������ͨ���������ա�
    break;
  case U8X8_MSG_BYTE_SET_DC:
    // ԭ��: �����Ϣ���� SPI ͨ���п��� Data/Command ѡ�����š�
    // ��������/������ (I2C ����Ҫ)
    // I2C ͨ���ض��Ŀ����ֽ� (0x00 �� 0x40) ������������ݣ���˸���Ϣ���� I2C �����塣
    break;
  case U8X8_MSG_BYTE_START_TRANSFER:
    // ԭ��: ���һ�� I2C �������еĿ�ʼ��
    buf_idx = 0;
    // �������������ñ��ػ�������������׼�������µ����ݿ顣
    break;
  case U8X8_MSG_BYTE_END_TRANSFER:
    // ԭ��: ���һ�� I2C �������еĽ�����
    // ��ʱ������ buffer ���Ѿ��ݴ���������Ҫ���͵����ݿ顣
    // ����ִ��ʵ�� I2C ���Ͳ��������ʱ����
    // ���ͻ������е�����
    // ע��: u8x8_GetI2CAddress(u8x8) ���ص��� 7 λ��ַ * 2 = 8 λ��ַ
    if (HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 100) != HAL_OK)
    {
      return 0; // ����ʧ��
    }
    break;
  default:
    return 0;
  }
  return 1;
}

/* ����ˢ�º��� */
void OLED_SendBuff(uint8_t buff[4][128])
{
  // ��ȡ u8g2 �Ļ�����ָ��
  uint8_t *u8g2_buffer = u8g2_GetBufferPtr(&u8g2);

  // �����ݿ����� u8g2 �Ļ�����
  memcpy(u8g2_buffer, buff, 4 * 128);

  // ���������������� OLED
  u8g2_SendBuffer(&u8g2);
}

 

/* Oled ��ʾ���� */
void oled_task(void)
{
  //  // --- ׼���׶� ---
  //  // ���û�ͼ��ɫ (���ڵ�ɫ����1 ͨ����ʾ��������)
  //  u8g2_SetDrawColor(&u8g2, 1);
  //  // ѡ��Ҫʹ�õ����� (ȷ�������ļ�����ӵ�����)
  //  u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr); // ncenB08: ������, _tr: ͸������

  //  // --- ���Ļ�ͼ���� ---
  //  // 1. ����ڴ滺���� (�ǳ���Ҫ��ÿ�λ�����֡ǰ�������)
  //  u8g2_ClearBuffer(&u8g2);

  //  // 2. ʹ�� u8g2 API �ڻ������л�ͼ
  //  //    ���л�ͼ������������ RAM �еĻ�������
  //  // �����ַ��� (����: u8g2ʵ��, x����, y����, �ַ���)
  //  // y ����ͨ�����ַ������ߵ�λ�á�
  //  u8g2_DrawStr(&u8g2, 2, 12, "Hello u8g2!"); // �� (2, 12) ��ʼ����
  //  u8g2_DrawStr(&u8g2, 2, 28, "Micron Elec Studio"); // ���Ƶڶ���

  //  // ����ͼ�� (ʾ����һ������Բ��һ��ʵ�Ŀ�)
  //  // ����Բ (����: u8g2ʵ��, Բ��x, Բ��y, �뾶, ����ѡ��)
  //  u8g2_DrawCircle(&u8g2, 90, 19, 10, U8G2_DRAW_ALL); // U8G2_DRAW_ALL ��Բ��
  //  // ����ʵ�Ŀ� (����: u8g2ʵ��, ���Ͻ�x, ���Ͻ�y, ���, �߶�)
  //  // u8g2_DrawBox(&u8g2, 50, 15, 20, 10);
  //  // ���ƿ��Ŀ� (����: u8g2ʵ��, ���Ͻ�x, ���Ͻ�y, ���, �߶�)
  //  // u8g2_DrawFrame(&u8g2, 50, 15, 20, 10);

  //  // 3. ������������һ���Է��͵���Ļ (�ǳ���Ҫ)
  //  //    ����������������֮ǰ��д�� I2C �ص������������������������ݷ��ͳ�ȥ��
  //  u8g2_SendBuffer(&u8g2);
 
    static uint8_t last_state = 0xFF; // ��ʼֵ��Ϊ�����ܵ�ֵ
    
    // ֻ����Ҫ���»�״̬�ı�ʱ��ˢ��
    if(oled_update_flag || (last_state != oled_display_state))
    {
        OLED_Clear();
        
        switch(oled_display_state)
        {
            case 0:
                oled_printf(0, 0, "WuZhaowei:19");  // ��ʾWuZhaowei:19
						    oled_printf(0, 1, "Wuwei:19");  // ��ʾWuZhaowei:19
						

                break;
            case 1:
                oled_printf(32, 1, "������");      // ��ʾZhoujing
                break;
            case 2:
                oled_printf(32, 1, "3638");          // ��ʾ3638
                break;
            case 3:
                oled_printf(32, 1, "1 5 1");         // ��ʾ1 5 1
                break;
            default:
                break;
        }
        
        last_state = oled_display_state;
        oled_update_flag = 0; // ������±�־
    
}
  
}
