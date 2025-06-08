#include "waveform_analyzer_app.h"
#include "adc_app.h"
#include <string.h>

// --- Copyright ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.
// Adapted for GD32F4xx standard library

// --- 全局变量定义 ---
float fft_input_buffer[FFT_SIZE];
complex_t fft_output_buffer[FFT_SIZE];
float magnitude_buffer[FFT_SIZE / 2];
float phase_buffer[FFT_SIZE / 2];

// --- 私有变量 ---
static uint32_t current_sample_rate = SAMPLE_RATE_HZ;
static uint8_t analyzer_initialized = 0;

// --- 私有函数原型 ---
static void simple_fft(complex_t *data, int n);
static void bit_reverse(complex_t *data, int n);
static int find_peak_frequency_index(const float *magnitude, int size);
static float calculate_rms(const float *data, int size);
static float detect_fundamental_frequency(const float *magnitude, int size);
static ADC_WaveformType classify_waveform_by_harmonics(const float *magnitude, int size, float fundamental_freq);

// --- 私有函数实现 ---

// 简单的FFT实现（基于Cooley-Tukey算法）
static void simple_fft(complex_t *data, int n)
{
    if (n <= 1) return;
    
    // 位反转
    bit_reverse(data, n);
    
    // FFT计算
    for (int len = 2; len <= n; len <<= 1)
    {
        float angle = -2.0f * M_PI / len;
        complex_t wlen = {cosf(angle), sinf(angle)};
        
        for (int i = 0; i < n; i += len)
        {
            complex_t w = {1.0f, 0.0f};
            
            for (int j = 0; j < len / 2; j++)
            {
                complex_t u = data[i + j];
                complex_t v = {
                    data[i + j + len / 2].real * w.real - data[i + j + len / 2].imag * w.imag,
                    data[i + j + len / 2].real * w.imag + data[i + j + len / 2].imag * w.real
                };
                
                data[i + j].real = u.real + v.real;
                data[i + j].imag = u.imag + v.imag;
                data[i + j + len / 2].real = u.real - v.real;
                data[i + j + len / 2].imag = u.imag - v.imag;
                
                // 更新w
                float temp = w.real * wlen.real - w.imag * wlen.imag;
                w.imag = w.real * wlen.imag + w.imag * wlen.real;
                w.real = temp;
            }
        }
    }
}

// 位反转
static void bit_reverse(complex_t *data, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++)
    {
        int bit = n >> 1;
        while (j & bit)
        {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        
        if (i < j)
        {
            complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }
}

// 查找峰值频率索引
static int find_peak_frequency_index(const float *magnitude, int size)
{
    int peak_index = 1; // 跳过DC分量
    float max_magnitude = magnitude[1];
    
    for (int i = 2; i < size; i++)
    {
        if (magnitude[i] > max_magnitude)
        {
            max_magnitude = magnitude[i];
            peak_index = i;
        }
    }
    
    return peak_index;
}

// 计算RMS值
static float calculate_rms(const float *data, int size)
{
    float sum = 0.0f;
    for (int i = 0; i < size; i++)
    {
        sum += data[i] * data[i];
    }
    return sqrtf(sum / size);
}

// 检测基波频率
static float detect_fundamental_frequency(const float *magnitude, int size)
{
    int peak_index = find_peak_frequency_index(magnitude, size);
    return (float)peak_index * current_sample_rate / (2 * size);
}

// 通过谐波分析分类波形
static ADC_WaveformType classify_waveform_by_harmonics(const float *magnitude, int size, float fundamental_freq)
{
    if (fundamental_freq < 1.0f) return ADC_WAVEFORM_DC;
    
    int fundamental_index = (int)(fundamental_freq * 2 * size / current_sample_rate);
    if (fundamental_index >= size) return ADC_WAVEFORM_UNKNOWN;
    
    float fundamental_amp = magnitude[fundamental_index];
    
    // 检查三次谐波
    int third_harmonic_index = fundamental_index * 3;
    float third_harmonic_ratio = 0.0f;
    if (third_harmonic_index < size)
    {
        third_harmonic_ratio = magnitude[third_harmonic_index] / fundamental_amp;
    }
    
    // 检查五次谐波
    int fifth_harmonic_index = fundamental_index * 5;
    float fifth_harmonic_ratio = 0.0f;
    if (fifth_harmonic_index < size)
    {
        fifth_harmonic_ratio = magnitude[fifth_harmonic_index] / fundamental_amp;
    }
    
    // 检查偶次谐波
    int second_harmonic_index = fundamental_index * 2;
    float second_harmonic_ratio = 0.0f;
    if (second_harmonic_index < size)
    {
        second_harmonic_ratio = magnitude[second_harmonic_index] / fundamental_amp;
    }
    
    // 波形分类逻辑
    if (third_harmonic_ratio > 0.3f || fifth_harmonic_ratio > 0.2f)
    {
        if (second_harmonic_ratio < 0.1f)
        {
            return ADC_WAVEFORM_SQUARE; // 方波：奇次谐波丰富，偶次谐波少
        }
        else
        {
            return ADC_WAVEFORM_TRIANGLE; // 三角波：谐波丰富
        }
    }
    else if (third_harmonic_ratio < 0.1f && fifth_harmonic_ratio < 0.05f)
    {
        return ADC_WAVEFORM_SINE; // 正弦波：谐波很少
    }
    
    return ADC_WAVEFORM_UNKNOWN;
}

// --- 公共函数实现 ---

int waveform_analyzer_init(uint32_t sample_rate)
{
    current_sample_rate = sample_rate;
    
    // 清零缓冲区
    memset(fft_input_buffer, 0, sizeof(fft_input_buffer));
    memset(fft_output_buffer, 0, sizeof(fft_output_buffer));
    memset(magnitude_buffer, 0, sizeof(magnitude_buffer));
    memset(phase_buffer, 0, sizeof(phase_buffer));
    
    analyzer_initialized = 1;
    return 0;
}

float Map_Input_To_FFT_Frequency(float input_frequency)
{
    return input_frequency * FFT_SIZE / current_sample_rate;
}

float Map_FFT_To_Input_Frequency(float fft_frequency)
{
    return fft_frequency * current_sample_rate / FFT_SIZE;
}

float Get_Waveform_Vpp(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *mean, float *rms)
{
    if (!adc_val_buffer || buffer_size == 0) return 0.0f;
    
    uint16_t min_val = adc_val_buffer[0];
    uint16_t max_val = adc_val_buffer[0];
    uint32_t sum = 0;
    uint64_t sum_squares = 0;
    
    for (uint16_t i = 0; i < buffer_size; i++)
    {
        uint16_t val = adc_val_buffer[i];
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
        sum += val;
        sum_squares += (uint64_t)val * val;
    }
    
    if (mean)
    {
        *mean = ADC_To_Voltage((float)sum / buffer_size);
    }
    
    if (rms)
    {
        float mean_squares = (float)sum_squares / buffer_size;
        float mean_val = (float)sum / buffer_size;
        float variance = mean_squares - mean_val * mean_val;
        *rms = ADC_To_Voltage(sqrtf(variance > 0 ? variance : 0));
    }
    
    return ADC_To_Voltage(max_val - min_val);
}

float Get_Waveform_Frequency(const uint16_t *adc_val_buffer, uint16_t buffer_size)
{
    if (!adc_val_buffer || buffer_size == 0) return 0.0f;
    
    // 执行FFT分析
    if (Perform_FFT(adc_val_buffer, buffer_size) != 0) return 0.0f;
    
    return detect_fundamental_frequency(magnitude_buffer, FFT_SIZE / 2);
}

ADC_WaveformType Get_Waveform_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size)
{
    if (!adc_val_buffer || buffer_size == 0) return ADC_WAVEFORM_UNKNOWN;
    
    float frequency = Get_Waveform_Frequency(adc_val_buffer, buffer_size);
    return classify_waveform_by_harmonics(magnitude_buffer, FFT_SIZE / 2, frequency);
}

int Perform_FFT(const uint16_t *adc_val_buffer, uint16_t buffer_size)
{
    if (!analyzer_initialized) return -1;
    if (!adc_val_buffer || buffer_size == 0) return -1;
    
    // 准备FFT输入数据
    uint16_t copy_size = (buffer_size < FFT_SIZE) ? buffer_size : FFT_SIZE;
    
    // 计算均值用于去除DC分量
    float mean = 0.0f;
    for (uint16_t i = 0; i < copy_size; i++)
    {
        mean += adc_val_buffer[i];
    }
    mean /= copy_size;
    
    // 复制数据并去除DC分量
    for (int i = 0; i < FFT_SIZE; i++)
    {
        if (i < copy_size)
        {
            fft_output_buffer[i].real = adc_val_buffer[i] - mean;
            fft_input_buffer[i] = fft_output_buffer[i].real;
        }
        else
        {
            fft_output_buffer[i].real = 0.0f;
            fft_input_buffer[i] = 0.0f;
        }
        fft_output_buffer[i].imag = 0.0f;
    }
    
    // 执行FFT
    simple_fft(fft_output_buffer, FFT_SIZE);
    
    // 计算幅度和相位
    for (int i = 0; i < FFT_SIZE / 2; i++)
    {
        magnitude_buffer[i] = sqrtf(fft_output_buffer[i].real * fft_output_buffer[i].real + 
                                   fft_output_buffer[i].imag * fft_output_buffer[i].imag);
        phase_buffer[i] = atan2f(fft_output_buffer[i].imag, fft_output_buffer[i].real);
    }
    
    return 0;
}

ADC_WaveformType Analyze_Frequency_And_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *signal_frequency)
{
    if (!adc_val_buffer || buffer_size == 0 || !signal_frequency) return ADC_WAVEFORM_UNKNOWN;
    
    *signal_frequency = Get_Waveform_Frequency(adc_val_buffer, buffer_size);
    return Get_Waveform_Type(adc_val_buffer, buffer_size);
}

float ADC_To_Voltage(uint16_t adc_value)
{
    return (float)adc_value * ADC_VREF_MV / ADC_MAX_VALUE / 1000.0f; // 转换为伏特
}

uint16_t Voltage_To_ADC(float voltage)
{
    return (uint16_t)(voltage * 1000.0f * ADC_MAX_VALUE / ADC_VREF_MV);
}

const char *GetWaveformTypeString(ADC_WaveformType waveform)
{
    switch (waveform)
    {
    case ADC_WAVEFORM_DC:       return "DC";
    case ADC_WAVEFORM_SINE:     return "Sine";
    case ADC_WAVEFORM_SQUARE:   return "Square";
    case ADC_WAVEFORM_TRIANGLE: return "Triangle";
    default:                    return "Unknown";
    }
}

int Set_Sample_Rate(uint32_t sample_rate)
{
    if (sample_rate == 0) return -1;
    current_sample_rate = sample_rate;
    return 0;
}

uint32_t Get_Sample_Rate(void)
{
    return current_sample_rate;
}

float Get_Waveform_Phase(const uint16_t *adc_val_buffer, uint16_t buffer_size, float frequency)
{
    if (!adc_val_buffer || buffer_size == 0 || frequency <= 0) return 0.0f;

    // 执行FFT分析
    if (Perform_FFT(adc_val_buffer, buffer_size) != 0) return 0.0f;

    // 找到基波频率对应的FFT索引
    int fundamental_index = (int)(frequency * FFT_SIZE / current_sample_rate);
    if (fundamental_index >= FFT_SIZE / 2) return 0.0f;

    return phase_buffer[fundamental_index];
}

float Get_Waveform_Phase_ZeroCrossing(const uint16_t *adc_val_buffer, uint16_t buffer_size, float frequency)
{
    if (!adc_val_buffer || buffer_size == 0 || frequency <= 0) return 0.0f;

    // 计算均值
    float mean = 0.0f;
    for (uint16_t i = 0; i < buffer_size; i++)
    {
        mean += adc_val_buffer[i];
    }
    mean /= buffer_size;

    // 查找第一个上升沿过零点
    for (uint16_t i = 1; i < buffer_size; i++)
    {
        if (adc_val_buffer[i-1] < mean && adc_val_buffer[i] >= mean)
        {
            // 找到过零点，计算相位
            float time_to_zero = (float)i / current_sample_rate;
            float phase = 2.0f * M_PI * frequency * time_to_zero;
            return fmodf(phase, 2.0f * M_PI);
        }
    }

    return 0.0f;
}

float Calculate_Phase_Difference(float phase1, float phase2)
{
    float diff = phase1 - phase2;

    // 将相位差限制在 [-π, π] 范围内
    while (diff > M_PI) diff -= 2.0f * M_PI;
    while (diff < -M_PI) diff += 2.0f * M_PI;

    return diff;
}

float Get_Phase_Difference(const uint16_t *adc_val_buffer1, const uint16_t *adc_val_buffer2,
                          uint16_t buffer_size, float frequency)
{
    if (!adc_val_buffer1 || !adc_val_buffer2 || buffer_size == 0) return 0.0f;

    float phase1 = Get_Waveform_Phase(adc_val_buffer1, buffer_size, frequency);
    float phase2 = Get_Waveform_Phase(adc_val_buffer2, buffer_size, frequency);

    return Calculate_Phase_Difference(phase1, phase2);
}

int Analyze_Harmonics(const uint16_t *adc_val_buffer, uint16_t buffer_size, WaveformInfo *waveform_info)
{
    if (!adc_val_buffer || buffer_size == 0 || !waveform_info) return -1;

    // 执行FFT分析
    if (Perform_FFT(adc_val_buffer, buffer_size) != 0) return -1;

    // 检测基波频率
    float fundamental_freq = detect_fundamental_frequency(magnitude_buffer, FFT_SIZE / 2);
    int fundamental_index = (int)(fundamental_freq * FFT_SIZE / current_sample_rate);

    if (fundamental_index >= FFT_SIZE / 2) return -1;

    float fundamental_amp = magnitude_buffer[fundamental_index];

    // 分析三次谐波
    int third_harmonic_index = fundamental_index * 3;
    if (third_harmonic_index < FFT_SIZE / 2)
    {
        waveform_info->third_harmonic.frequency = fundamental_freq * 3;
        waveform_info->third_harmonic.amplitude = magnitude_buffer[third_harmonic_index];
        waveform_info->third_harmonic.phase = phase_buffer[third_harmonic_index];
        waveform_info->third_harmonic.relative_amp = magnitude_buffer[third_harmonic_index] / fundamental_amp;
    }
    else
    {
        memset(&waveform_info->third_harmonic, 0, sizeof(HarmonicComponent));
    }

    // 分析五次谐波
    int fifth_harmonic_index = fundamental_index * 5;
    if (fifth_harmonic_index < FFT_SIZE / 2)
    {
        waveform_info->fifth_harmonic.frequency = fundamental_freq * 5;
        waveform_info->fifth_harmonic.amplitude = magnitude_buffer[fifth_harmonic_index];
        waveform_info->fifth_harmonic.phase = phase_buffer[fifth_harmonic_index];
        waveform_info->fifth_harmonic.relative_amp = magnitude_buffer[fifth_harmonic_index] / fundamental_amp;
    }
    else
    {
        memset(&waveform_info->fifth_harmonic, 0, sizeof(HarmonicComponent));
    }

    // 计算THD
    float harmonic_amps[10];
    int harmonic_count = 0;
    for (int h = 2; h <= 10 && fundamental_index * h < FFT_SIZE / 2; h++)
    {
        harmonic_amps[harmonic_count++] = magnitude_buffer[fundamental_index * h];
    }
    waveform_info->thd = Calculate_THD(fundamental_amp, harmonic_amps, harmonic_count);

    return 0;
}

float Get_Component_Phase(const complex_t *fft_buffer, int component_idx)
{
    if (!fft_buffer || component_idx < 0) return 0.0f;

    return atan2f(fft_buffer[component_idx].imag, fft_buffer[component_idx].real);
}

float Calculate_THD(float fundamental_amp, const float *harmonic_amps, int harmonic_count)
{
    if (fundamental_amp <= 0 || !harmonic_amps || harmonic_count <= 0) return 0.0f;

    float harmonic_sum_squares = 0.0f;
    for (int i = 0; i < harmonic_count; i++)
    {
        harmonic_sum_squares += harmonic_amps[i] * harmonic_amps[i];
    }

    return sqrtf(harmonic_sum_squares) / fundamental_amp * 100.0f; // 返回百分比
}

float Calculate_SNR(float signal_power, float noise_power)
{
    if (noise_power <= 0) return 0.0f;

    return 10.0f * log10f(signal_power / noise_power);
}

WaveformInfo Get_Waveform_Info(const uint16_t *adc_val_buffer, uint16_t buffer_size)
{
    WaveformInfo info;
    memset(&info, 0, sizeof(WaveformInfo));

    if (!adc_val_buffer || buffer_size == 0) return info;

    // 基本参数分析
    info.vpp = Get_Waveform_Vpp(adc_val_buffer, buffer_size, &info.mean, &info.rms);
    info.frequency = Get_Waveform_Frequency(adc_val_buffer, buffer_size);
    info.waveform_type = Get_Waveform_Type(adc_val_buffer, buffer_size);
    info.phase = Get_Waveform_Phase(adc_val_buffer, buffer_size, info.frequency);

    // 谐波分析
    Analyze_Harmonics(adc_val_buffer, buffer_size, &info);

    // 计算SNR（简化版本）
    float signal_power = info.rms * info.rms;
    float noise_power = signal_power * 0.01f; // 假设1%的噪声
    info.snr = Calculate_SNR(signal_power, noise_power);

    return info;
}
