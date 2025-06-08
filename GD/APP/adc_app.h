#ifndef __ADC_APP_H_
#define __ADC_APP_H_

#include "stdint.h"
#include "gd32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- 配置参数 ---
#define ADC_BUFFER_SIZE 1024                    // ADC缓冲区大小
#define ADC_RESOLUTION_BITS 12                  // ADC分辨率
#define ADC_MAX_VALUE ((1 << ADC_RESOLUTION_BITS) - 1)  // ADC最大值
#define ADC_VREF_MV 3300                        // ADC参考电压(毫伏)
#define ADC_TIMER_CLOCK_HZ 240000000            // ADC定时器时钟频率

// --- ADC采样模式 ---
typedef enum
{
    ADC_MODE_SINGLE,        // 单次采样
    ADC_MODE_CONTINUOUS,    // 连续采样
    ADC_MODE_TIMER_TRIGGER, // 定时器触发采样
    ADC_MODE_DMA_CIRCULAR   // DMA循环采样
} adc_mode_t;

// --- ADC状态 ---
typedef enum
{
    ADC_STATE_IDLE,         // 空闲状态
    ADC_STATE_SAMPLING,     // 采样中
    ADC_STATE_COMPLETE,     // 采样完成
    ADC_STATE_ERROR         // 错误状态
} adc_state_t;

// --- ADC配置结构体 ---
typedef struct
{
    adc_mode_t mode;                // 采样模式
    uint32_t sample_rate_hz;        // 采样频率(Hz)
    uint16_t buffer_size;           // 缓冲区大小
    uint8_t channel;                // ADC通道
    uint8_t trigger_source;         // 触发源
} adc_config_t;

// --- 全局变量声明 ---
extern uint8_t adc_recording_flag;     // ADC录制标志
extern char adc_filename[32];          // ADC文件名
extern uint16_t adc_buffer[ADC_BUFFER_SIZE];  // ADC数据缓冲区
extern volatile adc_state_t adc_current_state;  // ADC当前状态

// --- 高级ADC功能 ---
/**
 * @brief 初始化高级ADC功能
 * @param config: ADC配置参数
 * @retval 0: 成功, 非0: 失败
 */
int adc_app_init(const adc_config_t *config);

/**
 * @brief 启动ADC采样
 * @param samples: 采样点数 (0表示连续采样)
 * @retval 0: 成功, 非0: 失败
 */
int adc_app_start_sampling(uint16_t samples);

/**
 * @brief 停止ADC采样
 * @retval 0: 成功, 非0: 失败
 */
int adc_app_stop_sampling(void);

/**
 * @brief 配置ADC采样频率
 * @param sample_rate_hz: 采样频率(Hz)
 * @retval 0: 成功, 非0: 失败
 */
int adc_app_set_sample_rate(uint32_t sample_rate_hz);

/**
 * @brief 获取ADC采样频率
 * @retval uint32_t: 当前采样频率(Hz)
 */
uint32_t adc_app_get_sample_rate(void);

/**
 * @brief 配置ADC定时器触发
 * @param timer_freq_hz: 定时器频率(Hz)
 * @retval 0: 成功, 非0: 失败
 */
int adc_app_config_timer_trigger(uint32_t timer_freq_hz);

/**
 * @brief 获取ADC状态
 * @retval adc_state_t: 当前ADC状态
 */
adc_state_t adc_app_get_state(void);

/**
 * @brief 获取ADC数据缓冲区
 * @param buffer: 输出缓冲区指针
 * @param size: 缓冲区大小
 * @retval uint16_t: 实际获取的数据点数
 */
uint16_t adc_app_get_data(uint16_t *buffer, uint16_t size);

/**
 * @brief 将ADC原始值转换为电压(毫伏)
 * @param raw_value: ADC原始值
 * @retval uint16_t: 电压值(毫伏)
 */
uint16_t adc_app_raw_to_mv(uint16_t raw_value);

/**
 * @brief 将电压(毫伏)转换为ADC原始值
 * @param voltage_mv: 电压值(毫伏)
 * @retval uint16_t: ADC原始值
 */
uint16_t adc_app_mv_to_raw(uint16_t voltage_mv);

/**
 * @brief 获取ADC数据统计信息
 * @param buffer: 数据缓冲区
 * @param size: 数据大小
 * @param min: 输出最小值
 * @param max: 输出最大值
 * @param mean: 输出平均值
 * @retval 0: 成功, 非0: 失败
 */
int adc_app_get_statistics(const uint16_t *buffer, uint16_t size,
                          uint16_t *min, uint16_t *max, float *mean);

/**
 * @brief DMA传输完成回调函数
 */
void adc_app_dma_complete_callback(void);

/**
 * @brief DMA传输半完成回调函数
 */
void adc_app_dma_half_complete_callback(void);

// --- 原有函数保持兼容 ---
void adc_task(void);
void adc_start_recording(void);
void adc_stop_recording(void);
void adc_recording_init(void);
void adc_check_filesystem(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_APP_H_ */
