#include "led_app.h"
#include "gpio.h"
#include "math.h"

uint8_t ucLed[6] = {1,0,1,0,1,0};  // LED ״̬����

// LED ��˸��������
static LED_Blink_TypeDef led_blink_ctrl[6] = {0};

/**
 * @brief ��ʾ LED ״̬
 *
 *
 * @param ucLed Led ��������
 */
void led_disp(uint8_t *ucLed)
{
    // ��¼֮ǰ LED ״̬�仯ʱ��ʱ��
    uint8_t temp = 0x00;
    // ��¼֮ǰ LED ״̬�仯ǰ�Ƿ���Ҫ��ʾ
    static uint8_t temp_old = 0xff;

    for (int i = 0; i < 6; i++)
    {
        // �� LED ״̬д�� temp ��
        if (ucLed[i]) temp |= (1<<i); // �� i λ�� 1
    }

    // ���֮ǰ״̬��֮ǰ״̬��ͬ������ʾ
    if (temp_old != temp)
    {
        // ��ʼ�� GPIO �������
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, (temp & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 0
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, (temp & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 1
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, (temp & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 2
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, (temp & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 3
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12,  (temp & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 4
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,  (temp & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 5
        
        // ��¼��ǰ״̬
        temp_old = temp;
    }
}

/**
 * @brief ����ָ�� LED ����˸
 * @param led_index LED ���� (0-5)
 * @param on_time ����ʱ�� (ms)
 * @param off_time Ϩ��ʱ�� (ms)
 */
void led_blink_start(uint8_t led_index, uint16_t on_time, uint16_t off_time)
{
    if (led_index < 6) {
        led_blink_ctrl[led_index].led_index = led_index;
        led_blink_ctrl[led_index].on_time = on_time;
        led_blink_ctrl[led_index].off_time = off_time;
        led_blink_ctrl[led_index].is_blinking = 1;
        led_blink_ctrl[led_index].last_toggle = HAL_GetTick();
        ucLed[led_index] = 1;  // ��ʼ״̬��Ϊ����
    }
}

/**
 * @brief ָֹͣ�� LED ����˸
 * @param led_index LED ���� (0-5)
 */
void led_blink_stop(uint8_t led_index)
{
    if (led_index < 6) {
        led_blink_ctrl[led_index].is_blinking = 0;
    }
}

/**
 * @brief ���� LED ��˸
 * ��Ҫ����ѭ���ж��ڵ��ô˺���
 */
void led_blink_process(void)
{
    uint32_t current_time = HAL_GetTick();
    
    for (int i = 0; i < 6; i++) {
        if (led_blink_ctrl[i].is_blinking) {
            uint32_t elapsed = current_time - led_blink_ctrl[i].last_toggle;
            
            if (ucLed[i] == 1 && elapsed >= led_blink_ctrl[i].on_time) {
                ucLed[i] = 0;
                led_blink_ctrl[i].last_toggle = current_time;
            }
            else if (ucLed[i] == 0 && elapsed >= led_blink_ctrl[i].off_time) {
                ucLed[i] = 1;
                led_blink_ctrl[i].last_toggle = current_time;
            }
        }
    }
}

/**
 * @brief LED ��ʾ����
 *
 * ÿ�ε��ô˺���ʱ��LED ��ʾ���� ucLed �е�ֵ�����ڸ��� LED ��ʾ
 * �������ڿ��� LED ����ʾ��Ҳ�������ڿ��� LED ����˸
 */
void led_task(void)
{
    // ���� LED ��˸
    led_blink_process();
    
    // ���� LED ��ʾ
    led_disp(ucLed);
}

/*

void led_proc(void)
{
    // ���� LED ��˸
    static uint32_t breathCounter = 0;      // ����������
    static uint8_t pwmCounter = 0;          // PWM ������
    static const uint16_t breathPeriod = 4000; // �������� (ms)
    static const uint8_t pwmMax = 10;       // PWM ���ֵ
    static const float phaseShift = 3.14159f / 3.0f; // ��λƫ�ƣ���/3����Լ����2LEDͬʱ��
    
    // ��������������
    breathCounter = (breathCounter + 1) % breathPeriod;
    
    // ���� PWM ֵ (0-pwmMax)
    pwmCounter = (pwmCounter + 1) % pwmMax;
    
    // Ϊÿ��LED���ɾ�̬���ȣ���������仯
    for(uint8_t i = 0; i < 6; i++) 
    {
        // ���ݽǶȼ��㣬ʵ��˳����ʾЧ��
        float angle = (2.0f * 3.14159f * breathCounter) / breathPeriod - i * phaseShift;
        // ʹ�����Һ�����ȷ����̬����
        float sinValue = sinf(angle);
        // ������ǿֵ��ȷ����̬����
        float enhancedValue = sinValue > 0 ? powf(sinValue, 0.5f) : -powf(-sinValue, 0.5f);
        // ��ǿֵ����ȡ��ֵ��ȷ����̬����
        enhancedValue = enhancedValue > 0.7f ? enhancedValue : enhancedValue * 0.6f;
        // ��������ֵ(0-pwmMax)
        uint8_t brightness = (uint8_t)((enhancedValue + 1.0f) * pwmMax / 2.0f);
        // ���� LED ״̬
        ucLed[i] = (pwmCounter < brightness) ? 1 : 0;
    }

    // ���� LED ��ʾ
    led_disp(ucLed);
}

*/


