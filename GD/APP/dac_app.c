#include "dac_app.h"
#include "mcu_cmic_gd32f470vet6.h"
#include "systick.h"
#include <string.h>

// --- Copyright ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.
// Adapted for GD32F4xx standard library

// --- 私有变量 ---
static uint16_t waveform_buffer[WAVEFORM_SAMPLES];                // 存储当前波形数据的缓冲区
static dac_waveform_t current_waveform = WAVEFORM_SINE;           // 当前输出的波形类型
static uint32_t current_frequency_hz = 1000;                      // 当前波形频率 (Hz)
static uint16_t current_peak_amplitude_mv = DAC_VREF_MV / 2;      // 当前峰值幅度 (mV)
static uint16_t dac_amplitude_raw = DAC_MAX_VALUE / 2;            // 当前峰值幅度对应的 DAC 原始值
static uint8_t adc_sync_enabled = ADC_DAC_SYNC_ENABLE;            // ADC同步状态
static uint8_t adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER; // ADC采样频率倍数
static uint8_t zero_based_waveform = 1;                           // 是否基于零点的波形
static uint8_t dac_running = 0;                                   // DAC运行状态

// --- 私有函数原型 ---
static void generate_waveform(void);                       // 根据当前设置生成波形数据到缓冲区
static int update_timer_frequency(void);                   // 更新定时器周期以匹配当前频率
static int start_dac_dma(void);                           // 启动 DAC DMA 输出
static int stop_dac_dma(void);                            // 停止 DAC DMA 输出
static void generate_sine(uint16_t amp_raw);               // 生成正弦波数据
static void generate_square(uint16_t amp_raw);             // 生成方波数据
static void generate_triangle(uint16_t amp_raw);           // 生成三角波数据
static int update_adc_timer_frequency(void);              // 更新ADC定时器频率以匹配同步要求

// --- 私有函数实现 ---

// 生成正弦波数据
static void generate_sine(uint16_t amp_raw)
{
    uint16_t center_val = DAC_MAX_VALUE / 2;
    
    for (int i = 0; i < WAVEFORM_SAMPLES; i++)
    {
        float angle = 2.0f * M_PI * i / WAVEFORM_SAMPLES;
        float sine_val = sinf(angle);
        
        if (zero_based_waveform)
        {
            // 基于零点：0 到 amp_raw*2
            waveform_buffer[i] = (uint16_t)((sine_val + 1.0f) * amp_raw);
        }
        else
        {
            // 基于中点：center_val ± amp_raw
            waveform_buffer[i] = (uint16_t)(center_val + sine_val * amp_raw);
        }
        
        // 确保不超出DAC范围
        if (waveform_buffer[i] > DAC_MAX_VALUE)
            waveform_buffer[i] = DAC_MAX_VALUE;
    }
}

// 生成方波数据
static void generate_square(uint16_t amp_raw)
{
    uint16_t center_val = DAC_MAX_VALUE / 2;
    uint16_t high_val, low_val;
    
    if (zero_based_waveform)
    {
        high_val = amp_raw * 2;
        low_val = 0;
    }
    else
    {
        high_val = center_val + amp_raw;
        low_val = center_val - amp_raw;
    }
    
    // 确保不超出DAC范围
    if (high_val > DAC_MAX_VALUE) high_val = DAC_MAX_VALUE;
    if (low_val > DAC_MAX_VALUE) low_val = 0;
    
    int half_samples = WAVEFORM_SAMPLES / 2;
    
    for (int i = 0; i < half_samples; i++)
    {
        waveform_buffer[i] = high_val;
    }
    for (int i = half_samples; i < WAVEFORM_SAMPLES; i++)
    {
        waveform_buffer[i] = low_val;
    }
}

// 生成三角波数据
static void generate_triangle(uint16_t amp_raw)
{
    uint16_t center_val = DAC_MAX_VALUE / 2;
    uint16_t high_val, low_val;
    
    if (zero_based_waveform)
    {
        high_val = amp_raw * 2;
        low_val = 0;
    }
    else
    {
        high_val = center_val + amp_raw;
        low_val = center_val - amp_raw;
    }
    
    // 确保不超出DAC范围
    if (high_val > DAC_MAX_VALUE) high_val = DAC_MAX_VALUE;
    if (low_val > DAC_MAX_VALUE) low_val = 0;
    
    int half_samples = WAVEFORM_SAMPLES / 2;
    
    // 上升沿
    for (int i = 0; i < half_samples; i++)
    {
        waveform_buffer[i] = low_val + (uint16_t)((float)(high_val - low_val) * i / half_samples);
    }
    
    // 下降沿
    for (int i = half_samples; i < WAVEFORM_SAMPLES; i++)
    {
        waveform_buffer[i] = high_val - (uint16_t)((float)(high_val - low_val) * (i - half_samples) / half_samples);
    }
    
    // 确保起始和结束点正确
    if (WAVEFORM_SAMPLES > 0)
        waveform_buffer[0] = low_val;
    if (WAVEFORM_SAMPLES > 1)
        waveform_buffer[WAVEFORM_SAMPLES - 1] = low_val;
}

// 根据当前设置生成波形
static void generate_waveform(void)
{
    switch (current_waveform)
    {
    case WAVEFORM_SINE:
        generate_sine(dac_amplitude_raw);
        break;
    case WAVEFORM_SQUARE:
        generate_square(dac_amplitude_raw);
        break;
    case WAVEFORM_TRIANGLE:
        generate_triangle(dac_amplitude_raw);
        break;
    default:
        generate_sine(dac_amplitude_raw);
        break; // 默认为正弦波
    }
}

// 更新定时器频率
static int update_timer_frequency(void)
{
    if (current_frequency_hz == 0) return -1;
    
    uint32_t dac_update_freq = current_frequency_hz * WAVEFORM_SAMPLES;
    uint32_t timer_period = TIMER_INPUT_CLOCK_HZ / dac_update_freq;
    
    if (timer_period == 0) timer_period = 1;
    if (timer_period > 65535) timer_period = 65535;
    
    // 停止定时器
    timer_disable(TIMER5);
    
    // 重新配置定时器
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = 239;  // 预分频器，使时钟为1MHz
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (timer_period / 240) - 1;  // 调整周期
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    
    timer_init(TIMER5, &timer_initpara);
    timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);
    
    return 0;
}

// 启动DAC DMA输出
static int start_dac_dma(void)
{
    if (update_timer_frequency() != 0) return -1;
    
    // 更新DMA缓冲区地址和大小
    dma_channel_disable(DMA0, DMA_CH5);
    
    // 等待DMA通道禁用完成
    while (dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_FTF));
    
    // 重新配置DMA
    DMA_CHPADDR(DMA0, DMA_CH5) = DAC0_R12DH_ADDRESS;
    DMA_CHM0ADDR(DMA0, DMA_CH5) = (uint32_t)waveform_buffer;
    DMA_CHCNT(DMA0, DMA_CH5) = WAVEFORM_SAMPLES;
    
    // 启用DMA通道
    dma_channel_enable(DMA0, DMA_CH5);
    
    // 启动定时器
    timer_enable(TIMER5);
    
    dac_running = 1;
    
    // 如果启用ADC同步，更新ADC定时器
    if (adc_sync_enabled)
    {
        update_adc_timer_frequency();
    }
    
    return 0;
}

// 停止DAC DMA输出
static int stop_dac_dma(void)
{
    // 停止定时器
    timer_disable(TIMER5);
    
    // 停止DMA
    dma_channel_disable(DMA0, DMA_CH5);
    
    dac_running = 0;
    
    return 0;
}

// 更新ADC定时器频率以匹配同步要求
static int update_adc_timer_frequency(void)
{
    // 这里需要根据具体的ADC定时器配置来实现
    // 暂时返回成功
    return 0;
}

// --- 公共 API 函数实现 ---
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv)
{
    current_frequency_hz = (initial_freq_hz == 0) ? 1 : initial_freq_hz;
    dac_app_set_amplitude(initial_peak_amplitude_mv);
    generate_waveform();
    
    // 初始化ADC同步配置
    adc_sync_enabled = ADC_DAC_SYNC_ENABLE;
    adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER;
    
    // 初始化硬件
    bsp_dac_init();
    
    start_dac_dma();
}

int dac_app_set_waveform(dac_waveform_t type)
{
    if (stop_dac_dma() != 0) return -1;
    
    current_waveform = type;
    generate_waveform();
    
    return start_dac_dma();
}

int dac_app_set_frequency(uint32_t freq_hz)
{
    if (freq_hz == 0) return -1;
    if (stop_dac_dma() != 0) return -1;
    
    current_frequency_hz = freq_hz;
    
    return start_dac_dma();
}

int dac_app_set_amplitude(uint16_t peak_amplitude_mv)
{
    if (stop_dac_dma() != 0) return -1;
    
    uint16_t max_amplitude_mv = DAC_VREF_MV / 2;
    current_peak_amplitude_mv = (peak_amplitude_mv > max_amplitude_mv) ? max_amplitude_mv : peak_amplitude_mv;
    
    // 将 mV 幅度转换为 DAC 原始步进值
    dac_amplitude_raw = (uint16_t)(((float)current_peak_amplitude_mv / max_amplitude_mv) * (DAC_MAX_VALUE / 2.0f));
    if (dac_amplitude_raw > (DAC_MAX_VALUE / 2))
        dac_amplitude_raw = DAC_MAX_VALUE / 2;
    
    generate_waveform();
    
    return start_dac_dma();
}

uint16_t dac_app_get_amplitude(void)
{
    return current_peak_amplitude_mv;
}

uint32_t dac_app_get_frequency(void)
{
    return current_frequency_hz;
}

dac_waveform_t dac_app_get_waveform(void)
{
    return current_waveform;
}

uint32_t dac_app_get_update_frequency(void)
{
    return current_frequency_hz * WAVEFORM_SAMPLES;
}

int dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier)
{
    if (multiplier == 0) return -1;
    
    adc_sync_enabled = enable;
    adc_sampling_multiplier = multiplier;
    
    if (enable && dac_running)
    {
        return update_adc_timer_frequency();
    }
    
    return 0;
}

float dac_app_get_adc_sampling_interval_us(void)
{
    if (!adc_sync_enabled || current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0 || adc_sampling_multiplier == 0)
    {
        return 0.0f;
    }
    
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    uint64_t adc_sample_freq = dac_update_freq * adc_sampling_multiplier;
    
    if (adc_sample_freq == 0) return 0.0f;
    
    return 1000000.0f / (float)adc_sample_freq;
}

int dac_app_set_zero_based(uint8_t enable)
{
    if (stop_dac_dma() != 0) return -1;
    
    zero_based_waveform = enable ? 1 : 0;
    generate_waveform();
    
    return start_dac_dma();
}

uint8_t dac_app_get_zero_based(void)
{
    return zero_based_waveform;
}

int dac_app_start(void)
{
    return start_dac_dma();
}

int dac_app_stop(void)
{
    return stop_dac_dma();
}

// 原有函数保持兼容
void dac_task(void)
{
    // 可以在这里添加周期性任务，如状态监控等
}
