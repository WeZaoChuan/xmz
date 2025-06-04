#include "led_app.h"
#include "gpio.h"
#include "math.h"

uint8_t ucLed[6] = {1,0,1,0,1,0};  // LED 状态数组

// LED 闪烁控制数组
static LED_Blink_TypeDef led_blink_ctrl[6] = {0};

/**
 * @brief 显示 LED 状态
 *
 *
 * @param ucLed Led 数据数组
 */
void led_disp(uint8_t *ucLed)
{
    // 记录之前 LED 状态变化时的时间
    uint8_t temp = 0x00;
    // 记录之前 LED 状态变化前是否需要显示
    static uint8_t temp_old = 0xff;

    for (int i = 0; i < 6; i++)
    {
        // 将 LED 状态写入 temp 中
        if (ucLed[i]) temp |= (1<<i); // 将 i 位置 1
    }

    // 如果之前状态和之前状态相同，则不显示
    if (temp_old != temp)
    {
        // 初始化 GPIO 输出引脚
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, (temp & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 0
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, (temp & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 1
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, (temp & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 2
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, (temp & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 3
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12,  (temp & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 4
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13,  (temp & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 5
        
        // 记录当前状态
        temp_old = temp;
    }
}

/**
 * @brief 启动指定 LED 的闪烁
 * @param led_index LED 索引 (0-5)
 * @param on_time 点亮时间 (ms)
 * @param off_time 熄灭时间 (ms)
 */
void led_blink_start(uint8_t led_index, uint16_t on_time, uint16_t off_time)
{
    if (led_index < 6) {
        led_blink_ctrl[led_index].led_index = led_index;
        led_blink_ctrl[led_index].on_time = on_time;
        led_blink_ctrl[led_index].off_time = off_time;
        led_blink_ctrl[led_index].is_blinking = 1;
        led_blink_ctrl[led_index].last_toggle = HAL_GetTick();
        ucLed[led_index] = 1;  // 初始状态设为点亮
    }
}

/**
 * @brief 停止指定 LED 的闪烁
 * @param led_index LED 索引 (0-5)
 */
void led_blink_stop(uint8_t led_index)
{
    if (led_index < 6) {
        led_blink_ctrl[led_index].is_blinking = 0;
    }
}

/**
 * @brief 处理 LED 闪烁
 * 需要在主循环中定期调用此函数
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
 * @brief LED 显示任务
 *
 * 每次调用此函数时，LED 显示数组 ucLed 中的值将用于更新 LED 显示
 * 可以用于控制 LED 的显示，也可以用于控制 LED 的闪烁
 */
void led_task(void)
{
    // 处理 LED 闪烁
    led_blink_process();
    
    // 更新 LED 显示
    led_disp(ucLed);
}

/*

void led_proc(void)
{
    // 处理 LED 闪烁
    static uint32_t breathCounter = 0;      // 呼吸计数器
    static uint8_t pwmCounter = 0;          // PWM 计数器
    static const uint16_t breathPeriod = 4000; // 呼吸周期 (ms)
    static const uint8_t pwmMax = 10;       // PWM 最大值
    static const float phaseShift = 3.14159f / 3.0f; // 相位偏移（π/3），约等于2LED同时亮
    
    // 呼吸计数器递增
    breathCounter = (breathCounter + 1) % breathPeriod;
    
    // 计算 PWM 值 (0-pwmMax)
    pwmCounter = (pwmCounter + 1) % pwmMax;
    
    // 为每个LED生成静态亮度，不随呼吸变化
    for(uint8_t i = 0; i < 6; i++) 
    {
        // 根据角度计算，实现顺序显示效果
        float angle = (2.0f * 3.14159f * breathCounter) / breathPeriod - i * phaseShift;
        // 使用正弦函数，确保静态亮度
        float sinValue = sinf(angle);
        // 计算增强值，确保静态亮度
        float enhancedValue = sinValue > 0 ? powf(sinValue, 0.5f) : -powf(-sinValue, 0.5f);
        // 增强值，仅取正值，确保静态亮度
        enhancedValue = enhancedValue > 0.7f ? enhancedValue : enhancedValue * 0.6f;
        // 计算亮度值(0-pwmMax)
        uint8_t brightness = (uint8_t)((enhancedValue + 1.0f) * pwmMax / 2.0f);
        // 更新 LED 状态
        ucLed[i] = (pwmCounter < brightness) ? 1 : 0;
    }

    // 更新 LED 显示
    led_disp(ucLed);
}

*/


