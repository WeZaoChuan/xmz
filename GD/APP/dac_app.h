#ifndef __DAC_APP_H
#define __DAC_APP_H

#include "stdint.h"
#include "gd32f4xx.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- 配置参数 ---
#define DAC_RESOLUTION_BITS 12                         // DAC 分辨率 (位数)
#define DAC_MAX_VALUE ((1 << DAC_RESOLUTION_BITS) - 1) // DAC 最大数字值 (例如 12位 -> 4095)
#define DAC_VREF_MV 3300                               // DAC 参考电压 (毫伏)
#define WAVEFORM_SAMPLES 256                           // 每个波形周期的采样点数
#define TIMER_INPUT_CLOCK_HZ 240000000                 // 输入到 DAC 定时器的时钟频率 (Hz)

// --- ADC采样同步配置 ---
#define ADC_DAC_SYNC_ENABLE 1       // 是否启用ADC与DAC同步 (0:禁用, 1:启用)
#define ADC_SAMPLING_MULTIPLIER 1   // ADC采样频率相对于DAC更新频率的倍数

// --- 波形类型枚举 ---
typedef enum
{
    WAVEFORM_SINE,    // 正弦波
    WAVEFORM_SQUARE,  // 方波
    WAVEFORM_TRIANGLE // 三角波
} dac_waveform_t;

// --- 函数原型 ---
/**
 * @brief 初始化 DAC 应用库
 * @param initial_freq_hz: 初始波形频率 (Hz)
 * @param initial_peak_amplitude_mv: 初始波形峰值幅度 (毫伏, 相对中心电压 Vref/2)
 * @retval None
 */
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv);

/**
 * @brief 设置波形类型
 * @param type: 波形类型 (WAVEFORM_SINE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE)
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_set_waveform(dac_waveform_t type);

/**
 * @brief 设置波形频率
 * @param freq_hz: 波形频率 (Hz)
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_set_frequency(uint32_t freq_hz);

/**
 * @brief 设置波形峰值幅度
 * @param peak_amplitude_mv: 峰值幅度 (毫伏, 相对中心电压 Vref/2)
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_set_amplitude(uint16_t peak_amplitude_mv);

/**
 * @brief 获取当前波形峰值幅度
 * @retval uint16_t: 当前峰值幅度(毫伏)
 */
uint16_t dac_app_get_amplitude(void);

/**
 * @brief 获取当前波形频率
 * @retval uint32_t: 当前频率(Hz)
 */
uint32_t dac_app_get_frequency(void);

/**
 * @brief 获取当前波形类型
 * @retval dac_waveform_t: 当前波形类型
 */
dac_waveform_t dac_app_get_waveform(void);

/**
 * @brief 获取当前DAC更新频率，用于外部ADC同步配置
 * @retval uint32_t: DAC每秒更新点数 (Hz * WAVEFORM_SAMPLES)
 */
uint32_t dac_app_get_update_frequency(void);

/**
 * @brief 配置ADC同步参数
 * @param enable: 是否启用同步 (0:禁用, 1:启用)
 * @param multiplier: ADC采样频率相对于DAC更新频率的倍数
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier);

/**
 * @brief 获取计算出的ADC采样间隔
 * @retval float: ADC采样间隔(微秒)
 */
float dac_app_get_adc_sampling_interval_us(void);

/**
 * @brief 设置波形基准模式
 * @param enable: 是否基于零点(0:基于中点, 1:基于零点)
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_set_zero_based(uint8_t enable);

/**
 * @brief 获取当前波形基准模式
 * @retval uint8_t: 当前模式(0:基于中点, 1:基于零点)
 */
uint8_t dac_app_get_zero_based(void);

/**
 * @brief 启动DAC输出
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_start(void);

/**
 * @brief 停止DAC输出
 * @retval 0: 成功, 非0: 失败
 */
int dac_app_stop(void);

// 原有函数保持兼容
void dac_task(void);

#ifdef __cplusplus
}
#endif

#endif /* __DAC_APP_H */
