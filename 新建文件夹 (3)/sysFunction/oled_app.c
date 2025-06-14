#include "oled_app.h"
#include "adc_app.h"  // 引用ADC应用层头文件
extern RTC_HandleTypeDef hrtc; // 声明外部变量 hrtc

// 外部全局变量定义 (在btn_app.c中定义)
extern volatile uint8_t oled_display_state; // OLED显示状态：0-吴兆伟:19, 1-周静, 2-3638, 3-1 5 1
extern volatile uint8_t oled_update_flag;   // OLED更新标志

// 外部 ADC 相关变量定义
extern __IO uint32_t adc_val_buffer[2048];  // ADC DMA环形缓冲区

/**
 * @brief   在OLED上显示格式化字符串(6x8点阵ASCII)
 * @param x 水平位置(0-127点)
 * @param y 垂直位置(0-3行)
 * @param format 格式化字符串
 * @return 返回字符串长度
 * @example oled_printf(0, 0, "Data = %d", dat);
 **/
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
    char buffer[512]; // 格式化字符串缓冲区
    va_list arg;      // 可变参数列表
    int len;          // 格式化字符串长度

    va_start(arg, format);
    // 格式化字符串处理
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);

    OLED_ShowStr(x, y, buffer, 8);
    return len;
}
 





/* OLED任务 - 根据状态显示内容 */
void oled_task(void)
{
    static uint32_t last_timestamp = 0; // 上次更新时间戳
    static uint32_t last_adc = 0;       // 上次ADC值
    static uint8_t first_run = 1;       // 首次运行标志
    static uint32_t display_counter = 0;// 刷新计数器

    uint32_t current_timestamp = HAL_GetTick();
    uint32_t current_adc = adc_val_buffer[0];

    display_counter++;
    uint32_t adc_diff = (current_adc > last_adc) ? (current_adc - last_adc) : (last_adc - current_adc);

    if (first_run || (display_counter >= 10) || (adc_diff >= 5) || oled_update_flag) {
        if (first_run) {
            OLED_Clear();
            first_run = 0;
        }
        // 新增：oled_display_state==0时显示RTC时间
        if (oled_display_state == 0) {
            RTC_TimeTypeDef rtc_time;
            RTC_DateTypeDef rtc_date;
            HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "Time: %02d:%02d:%02d", rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
            oled_printf(0, 0, "%s", time_str);
        } else {
            // 其他状态下显示原有内容
            oled_printf(0, 0, "systick:%-10lu", current_timestamp);
            oled_printf(0, 1, "adc:%-10lu", current_adc);
        }
        last_timestamp = current_timestamp;
        last_adc = current_adc;
        display_counter = 0;
        oled_update_flag = 0;
    }
}