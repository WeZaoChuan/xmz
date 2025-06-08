#ifndef __WAVEFORM_ANALYZER_APP_H
#define __WAVEFORM_ANALYZER_APP_H

#include "stdint.h"
#include "gd32f4xx.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- 配置参数 ---
#define FFT_SIZE 256                    // FFT点数
#define SAMPLE_RATE_HZ 10000           // 默认采样率
#define FREQUENCY_RESOLUTION (SAMPLE_RATE_HZ / FFT_SIZE)  // 频率分辨率

// --- 波形类型定义 ---
typedef enum
{
    ADC_WAVEFORM_DC = 0,       // 直流信号
    ADC_WAVEFORM_SINE = 1,     // 正弦波
    ADC_WAVEFORM_SQUARE = 2,   // 方波
    ADC_WAVEFORM_TRIANGLE = 3, // 三角波
    ADC_WAVEFORM_UNKNOWN = 255 // 未知波形
} ADC_WaveformType;

// --- 谐波分量信息结构体 ---
typedef struct
{
    float frequency;    // 谐波频率
    float amplitude;    // 谐波幅度
    float phase;        // 谐波相位
    float relative_amp; // 相对于基波的幅度比
} HarmonicComponent;

// --- 扩展的波形信息结构体 ---
typedef struct
{
    ADC_WaveformType waveform_type;   // 波形类型枚举
    float frequency;                  // 基波频率，单位Hz
    float vpp;                        // 峰峰值，单位V
    float mean;                       // 均值，单位V
    float rms;                        // 有效值，单位V
    float phase;                      // 基波相位，单位弧度
    HarmonicComponent third_harmonic; // 三次谐波信息
    HarmonicComponent fifth_harmonic; // 五次谐波信息
    float thd;                        // 总谐波失真
    float snr;                        // 信噪比
} WaveformInfo;

// --- FFT相关结构体 ---
typedef struct
{
    float real;
    float imag;
} complex_t;

// --- 全局变量声明 ---
extern float fft_input_buffer[FFT_SIZE];
extern complex_t fft_output_buffer[FFT_SIZE];
extern float magnitude_buffer[FFT_SIZE / 2];
extern float phase_buffer[FFT_SIZE / 2];

// --- 初始化函数 ---
/**
 * @brief 初始化波形分析器
 * @param sample_rate: 采样率(Hz)
 * @retval 0: 成功, 非0: 失败
 */
int waveform_analyzer_init(uint32_t sample_rate);

// --- 频率映射函数 ---
/**
 * @brief 将输入频率映射到FFT频率
 * @param input_frequency: 输入频率
 * @retval float: FFT频率
 */
float Map_Input_To_FFT_Frequency(float input_frequency);

/**
 * @brief 将FFT频率映射到输入频率
 * @param fft_frequency: FFT频率
 * @retval float: 输入频率
 */
float Map_FFT_To_Input_Frequency(float fft_frequency);

// --- 基础波形参数分析函数 ---
/**
 * @brief 获取波形峰峰值、均值和有效值
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @param mean: 输出均值
 * @param rms: 输出有效值
 * @retval float: 峰峰值
 */
float Get_Waveform_Vpp(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *mean, float *rms);

/**
 * @brief 获取波形频率
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @retval float: 频率(Hz)
 */
float Get_Waveform_Frequency(const uint16_t *adc_val_buffer, uint16_t buffer_size);

/**
 * @brief 识别波形类型
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @retval ADC_WaveformType: 波形类型
 */
ADC_WaveformType Get_Waveform_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size);

// --- FFT分析函数 ---
/**
 * @brief 执行FFT变换
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @retval 0: 成功, 非0: 失败
 */
int Perform_FFT(const uint16_t *adc_val_buffer, uint16_t buffer_size);

/**
 * @brief 分析频率和波形类型
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @param signal_frequency: 输出信号频率
 * @retval ADC_WaveformType: 波形类型
 */
ADC_WaveformType Analyze_Frequency_And_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *signal_frequency);

// --- 相位分析函数 ---
/**
 * @brief 获取波形相位
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @param frequency: 信号频率
 * @retval float: 相位(弧度)
 */
float Get_Waveform_Phase(const uint16_t *adc_val_buffer, uint16_t buffer_size, float frequency);

/**
 * @brief 通过过零点检测获取波形相位
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @param frequency: 信号频率
 * @retval float: 相位(弧度)
 */
float Get_Waveform_Phase_ZeroCrossing(const uint16_t *adc_val_buffer, uint16_t buffer_size, float frequency);

/**
 * @brief 计算两个相位之间的差值
 * @param phase1: 相位1
 * @param phase2: 相位2
 * @retval float: 相位差
 */
float Calculate_Phase_Difference(float phase1, float phase2);

/**
 * @brief 获取两个信号之间的相位差
 * @param adc_val_buffer1: 第一个信号缓冲区
 * @param adc_val_buffer2: 第二个信号缓冲区
 * @param buffer_size: 缓冲区大小
 * @param frequency: 信号频率
 * @retval float: 相位差
 */
float Get_Phase_Difference(const uint16_t *adc_val_buffer1, const uint16_t *adc_val_buffer2, 
                          uint16_t buffer_size, float frequency);

// --- 谐波分析函数 ---
/**
 * @brief 分析谐波成分
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @param waveform_info: 输出波形信息
 * @retval 0: 成功, 非0: 失败
 */
int Analyze_Harmonics(const uint16_t *adc_val_buffer, uint16_t buffer_size, WaveformInfo *waveform_info);

/**
 * @brief 获取FFT分量的相位
 * @param fft_buffer: FFT缓冲区
 * @param component_idx: 分量索引
 * @retval float: 相位
 */
float Get_Component_Phase(const complex_t *fft_buffer, int component_idx);

/**
 * @brief 计算总谐波失真(THD)
 * @param fundamental_amp: 基波幅度
 * @param harmonic_amps: 谐波幅度数组
 * @param harmonic_count: 谐波数量
 * @retval float: THD值
 */
float Calculate_THD(float fundamental_amp, const float *harmonic_amps, int harmonic_count);

/**
 * @brief 计算信噪比(SNR)
 * @param signal_power: 信号功率
 * @param noise_power: 噪声功率
 * @retval float: SNR值(dB)
 */
float Calculate_SNR(float signal_power, float noise_power);

// --- 辅助函数 ---
/**
 * @brief 获取波形类型字符串
 * @param waveform: 波形类型
 * @retval const char*: 波形类型字符串
 */
const char *GetWaveformTypeString(ADC_WaveformType waveform);

/**
 * @brief 将ADC值转换为电压
 * @param adc_value: ADC值
 * @retval float: 电压值(V)
 */
float ADC_To_Voltage(uint16_t adc_value);

/**
 * @brief 将电压转换为ADC值
 * @param voltage: 电压值(V)
 * @retval uint16_t: ADC值
 */
uint16_t Voltage_To_ADC(float voltage);

// --- 综合接口 ---
/**
 * @brief 获取完整的波形信息
 * @param adc_val_buffer: ADC数据缓冲区
 * @param buffer_size: 缓冲区大小
 * @retval WaveformInfo: 波形信息结构体
 */
WaveformInfo Get_Waveform_Info(const uint16_t *adc_val_buffer, uint16_t buffer_size);

/**
 * @brief 设置采样率
 * @param sample_rate: 采样率(Hz)
 * @retval 0: 成功, 非0: 失败
 */
int Set_Sample_Rate(uint32_t sample_rate);

/**
 * @brief 获取当前采样率
 * @retval uint32_t: 采样率(Hz)
 */
uint32_t Get_Sample_Rate(void);

#ifdef __cplusplus
}
#endif

#endif // __WAVEFORM_ANALYZER_APP_H
