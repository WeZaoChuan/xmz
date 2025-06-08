#include "mcu_cmic_gd32f470vet6.h"

// --- 信号发生器和分析器演示程序 ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.

// --- 演示配置 ---
#define DEMO_FREQUENCY_HZ 1000      // 演示频率
#define DEMO_AMPLITUDE_MV 1000      // 演示幅度(毫伏)
#define DEMO_SAMPLE_RATE 10000      // 演示采样率

// --- 全局变量 ---
static uint8_t demo_running = 0;
static uint32_t demo_step = 0;

// --- 函数原型 ---
void signal_generator_demo_init(void);
void signal_generator_demo_task(void);
void waveform_analysis_demo(void);
void print_waveform_info(const WaveformInfo *info);

// --- 函数实现 ---

/**
 * @brief 初始化信号发生器演示
 */
void signal_generator_demo_init(void)
{
    my_printf(DEBUG_USART, "\r\n=== Signal Generator & Analyzer Demo ===\r\n");
    
    // 初始化DAC波形发生器
    my_printf(DEBUG_USART, "Initializing DAC waveform generator...\r\n");
    dac_app_init(DEMO_FREQUENCY_HZ, DEMO_AMPLITUDE_MV);
    
    // 初始化高级ADC功能
    my_printf(DEBUG_USART, "Initializing advanced ADC...\r\n");
    adc_config_t adc_config = {
        .mode = ADC_MODE_DMA_CIRCULAR,
        .sample_rate_hz = DEMO_SAMPLE_RATE,
        .buffer_size = ADC_BUFFER_SIZE,
        .channel = 0,
        .trigger_source = 0
    };
    
    if (adc_app_init(&adc_config) == 0)
    {
        my_printf(DEBUG_USART, "ADC initialized successfully\r\n");
    }
    else
    {
        my_printf(DEBUG_USART, "ADC initialization failed\r\n");
    }
    
    // 初始化波形分析器
    my_printf(DEBUG_USART, "Initializing waveform analyzer...\r\n");
    if (waveform_analyzer_init(DEMO_SAMPLE_RATE) == 0)
    {
        my_printf(DEBUG_USART, "Waveform analyzer initialized successfully\r\n");
    }
    else
    {
        my_printf(DEBUG_USART, "Waveform analyzer initialization failed\r\n");
    }
    
    // 初始化定时器
    bsp_timer0_init();
    
    demo_running = 1;
    demo_step = 0;
    
    my_printf(DEBUG_USART, "Demo initialization complete!\r\n");
    my_printf(DEBUG_USART, "Starting with sine wave at %d Hz, %d mV\r\n", 
              DEMO_FREQUENCY_HZ, DEMO_AMPLITUDE_MV);
}

/**
 * @brief 信号发生器演示任务
 */
void signal_generator_demo_task(void)
{
    if (!demo_running) return;
    
    static uint32_t last_demo_time = 0;
    uint32_t current_time = (uint32_t)get_system_ms();
    
    // 每5秒切换一次演示步骤
    if (current_time - last_demo_time >= 5000)
    {
        last_demo_time = current_time;
        
        switch (demo_step)
        {
        case 0:
            // 正弦波演示
            my_printf(DEBUG_USART, "\r\n--- Step 1: Sine Wave ---\r\n");
            dac_app_set_waveform(WAVEFORM_SINE);
            dac_app_set_frequency(DEMO_FREQUENCY_HZ);
            dac_app_set_amplitude(DEMO_AMPLITUDE_MV);
            my_printf(DEBUG_USART, "Generated: Sine wave, %d Hz, %d mV\r\n", 
                      DEMO_FREQUENCY_HZ, DEMO_AMPLITUDE_MV);
            break;
            
        case 1:
            // 方波演示
            my_printf(DEBUG_USART, "\r\n--- Step 2: Square Wave ---\r\n");
            dac_app_set_waveform(WAVEFORM_SQUARE);
            my_printf(DEBUG_USART, "Generated: Square wave, %d Hz, %d mV\r\n", 
                      dac_app_get_frequency(), dac_app_get_amplitude());
            break;
            
        case 2:
            // 三角波演示
            my_printf(DEBUG_USART, "\r\n--- Step 3: Triangle Wave ---\r\n");
            dac_app_set_waveform(WAVEFORM_TRIANGLE);
            my_printf(DEBUG_USART, "Generated: Triangle wave, %d Hz, %d mV\r\n", 
                      dac_app_get_frequency(), dac_app_get_amplitude());
            break;
            
        case 3:
            // 频率变化演示
            my_printf(DEBUG_USART, "\r\n--- Step 4: Frequency Change ---\r\n");
            dac_app_set_frequency(2000); // 改为2kHz
            my_printf(DEBUG_USART, "Generated: Triangle wave, %d Hz, %d mV\r\n", 
                      dac_app_get_frequency(), dac_app_get_amplitude());
            break;
            
        case 4:
            // 幅度变化演示
            my_printf(DEBUG_USART, "\r\n--- Step 5: Amplitude Change ---\r\n");
            dac_app_set_amplitude(500); // 改为500mV
            my_printf(DEBUG_USART, "Generated: Triangle wave, %d Hz, %d mV\r\n", 
                      dac_app_get_frequency(), dac_app_get_amplitude());
            break;
            
        case 5:
            // 波形分析演示
            my_printf(DEBUG_USART, "\r\n--- Step 6: Waveform Analysis ---\r\n");
            waveform_analysis_demo();
            break;
            
        default:
            // 重新开始
            demo_step = -1; // 下次循环会变成0
            my_printf(DEBUG_USART, "\r\n--- Demo Restart ---\r\n");
            break;
        }
        
        demo_step++;
    }
}

/**
 * @brief 波形分析演示
 */
void waveform_analysis_demo(void)
{
    my_printf(DEBUG_USART, "Starting waveform analysis...\r\n");
    
    // 启动ADC采样
    if (adc_app_start_sampling(ADC_BUFFER_SIZE) == 0)
    {
        my_printf(DEBUG_USART, "ADC sampling started\r\n");
        
        // 等待采样完成
        uint32_t timeout = (uint32_t)get_system_ms() + 1000; // 1秒超时
        while (adc_app_get_state() == ADC_STATE_SAMPLING && (uint32_t)get_system_ms() < timeout)
        {
            delay_1ms(10);
        }
        
        if (adc_app_get_state() == ADC_STATE_COMPLETE)
        {
            my_printf(DEBUG_USART, "ADC sampling completed\r\n");
            
            // 获取ADC数据
            uint16_t sample_buffer[ADC_BUFFER_SIZE];
            uint16_t samples_read = adc_app_get_data(sample_buffer, ADC_BUFFER_SIZE);
            
            if (samples_read > 0)
            {
                my_printf(DEBUG_USART, "Analyzing %d samples...\r\n", samples_read);
                
                // 执行波形分析
                WaveformInfo waveform_info = Get_Waveform_Info(sample_buffer, samples_read);
                
                // 打印分析结果
                print_waveform_info(&waveform_info);
                
                // 获取统计信息
                uint16_t min_val, max_val;
                float mean_val;
                if (adc_app_get_statistics(sample_buffer, samples_read, &min_val, &max_val, &mean_val) == 0)
                {
                    my_printf(DEBUG_USART, "Statistics: Min=%d, Max=%d, Mean=%.2f\r\n", 
                              min_val, max_val, mean_val);
                }
            }
            else
            {
                my_printf(DEBUG_USART, "No ADC data available\r\n");
            }
        }
        else
        {
            my_printf(DEBUG_USART, "ADC sampling timeout or error\r\n");
        }
        
        // 停止ADC采样
        adc_app_stop_sampling();
    }
    else
    {
        my_printf(DEBUG_USART, "Failed to start ADC sampling\r\n");
    }
}

/**
 * @brief 打印波形信息
 */
void print_waveform_info(const WaveformInfo *info)
{
    if (!info) return;
    
    my_printf(DEBUG_USART, "\r\n=== Waveform Analysis Results ===\r\n");
    my_printf(DEBUG_USART, "Type: %s\r\n", GetWaveformTypeString(info->waveform_type));
    my_printf(DEBUG_USART, "Frequency: %.2f Hz\r\n", info->frequency);
    my_printf(DEBUG_USART, "Peak-to-Peak: %.3f V\r\n", info->vpp);
    my_printf(DEBUG_USART, "Mean: %.3f V\r\n", info->mean);
    my_printf(DEBUG_USART, "RMS: %.3f V\r\n", info->rms);
    my_printf(DEBUG_USART, "Phase: %.2f rad (%.1f deg)\r\n", info->phase, info->phase * 180.0f / M_PI);
    
    if (info->third_harmonic.amplitude > 0)
    {
        my_printf(DEBUG_USART, "3rd Harmonic: %.2f Hz, %.1f%% amplitude\r\n", 
                  info->third_harmonic.frequency, info->third_harmonic.relative_amp * 100.0f);
    }
    
    if (info->fifth_harmonic.amplitude > 0)
    {
        my_printf(DEBUG_USART, "5th Harmonic: %.2f Hz, %.1f%% amplitude\r\n", 
                  info->fifth_harmonic.frequency, info->fifth_harmonic.relative_amp * 100.0f);
    }
    
    my_printf(DEBUG_USART, "THD: %.2f%%\r\n", info->thd);
    my_printf(DEBUG_USART, "SNR: %.1f dB\r\n", info->snr);
    my_printf(DEBUG_USART, "================================\r\n");
}

/**
 * @brief 停止演示
 */
void signal_generator_demo_stop(void)
{
    demo_running = 0;
    dac_app_stop();
    adc_app_stop_sampling();
    my_printf(DEBUG_USART, "Demo stopped\r\n");
}

/**
 * @brief 获取演示运行状态
 */
uint8_t signal_generator_demo_is_running(void)
{
    return demo_running;
}
