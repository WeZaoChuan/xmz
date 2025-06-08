# GD32F4xx 新增功能模块说明

本文档说明了在GD项目中新增的三个重要功能模块，这些模块参考HAL项目的实现，使用GD32标准库进行开发。

## 1. DAC波形发生器模块 (dac_app)

### 功能特性
- 支持正弦波、方波、三角波生成
- 可配置频率、幅度、波形类型
- DMA驱动的高性能输出
- ADC同步功能
- 零基准/中点基准模式切换

### 主要API
```c
// 初始化DAC应用
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv);

// 设置波形类型
int dac_app_set_waveform(dac_waveform_t type);

// 设置频率和幅度
int dac_app_set_frequency(uint32_t freq_hz);
int dac_app_set_amplitude(uint16_t peak_amplitude_mv);

// 启动/停止输出
int dac_app_start(void);
int dac_app_stop(void);
```

### 使用示例
```c
// 初始化：1kHz正弦波，1V幅度
dac_app_init(1000, 1000);

// 切换到方波
dac_app_set_waveform(WAVEFORM_SQUARE);

// 改变频率到2kHz
dac_app_set_frequency(2000);
```

## 2. 高级ADC功能模块 (adc_app)

### 功能特性
- 多种采样模式（单次、连续、定时器触发、DMA循环）
- 可配置采样频率
- DMA驱动的高速采集
- 定时器触发采样
- 数据统计分析功能

### 主要API
```c
// 初始化ADC
int adc_app_init(const adc_config_t *config);

// 启动/停止采样
int adc_app_start_sampling(uint16_t samples);
int adc_app_stop_sampling(void);

// 获取数据和状态
uint16_t adc_app_get_data(uint16_t *buffer, uint16_t size);
adc_state_t adc_app_get_state(void);

// 数据转换和统计
uint16_t adc_app_raw_to_mv(uint16_t raw_value);
int adc_app_get_statistics(const uint16_t *buffer, uint16_t size, 
                          uint16_t *min, uint16_t *max, float *mean);
```

### 使用示例
```c
// 配置ADC
adc_config_t config = {
    .mode = ADC_MODE_DMA_CIRCULAR,
    .sample_rate_hz = 10000,
    .buffer_size = 1024,
    .channel = 0
};

// 初始化并启动采样
adc_app_init(&config);
adc_app_start_sampling(1024);

// 获取数据
uint16_t buffer[1024];
uint16_t samples = adc_app_get_data(buffer, 1024);
```

## 3. 波形分析器模块 (waveform_analyzer_app)

### 功能特性
- FFT频谱分析
- 波形类型识别（DC、正弦波、方波、三角波）
- 频率、幅度、相位测量
- 谐波分析（三次、五次谐波）
- RMS、峰峰值、均值计算
- 总谐波失真(THD)和信噪比(SNR)计算

### 主要API
```c
// 初始化分析器
int waveform_analyzer_init(uint32_t sample_rate);

// 基础分析
float Get_Waveform_Frequency(const uint16_t *adc_val_buffer, uint16_t buffer_size);
ADC_WaveformType Get_Waveform_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size);
float Get_Waveform_Vpp(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *mean, float *rms);

// FFT分析
int Perform_FFT(const uint16_t *adc_val_buffer, uint16_t buffer_size);

// 综合分析
WaveformInfo Get_Waveform_Info(const uint16_t *adc_val_buffer, uint16_t buffer_size);
```

### 使用示例
```c
// 初始化分析器
waveform_analyzer_init(10000);

// 分析ADC数据
uint16_t adc_data[256];
WaveformInfo info = Get_Waveform_Info(adc_data, 256);

printf("频率: %.2f Hz\n", info.frequency);
printf("波形类型: %s\n", GetWaveformTypeString(info.waveform_type));
printf("峰峰值: %.3f V\n", info.vpp);
printf("THD: %.2f%%\n", info.thd);
```

## 4. 演示程序 (signal_generator_demo)

### 功能说明
提供了一个完整的演示程序，展示如何使用上述三个模块：
- 自动循环演示不同波形类型
- 实时波形分析
- 参数动态调整

### 使用方法
```c
// 在main函数中初始化
signal_generator_demo_init();

// 在主循环中调用
while(1) {
    signal_generator_demo_task();
    // 其他任务...
}
```

## 5. 硬件配置

### DAC配置
- 使用DAC0通道0 (PA4)
- TIMER5作为触发源
- DMA0通道5用于数据传输

### ADC配置
- 使用ADC0通道10 (PC0)
- TIMER0作为触发源
- DMA1通道0用于数据传输

### 定时器配置
- TIMER5: DAC触发定时器
- TIMER0: ADC触发定时器

## 6. 编译和集成

### 文件列表
新增的文件需要添加到项目中：
```
GD/APP/dac_app.h
GD/APP/dac_app.c
GD/APP/waveform_analyzer_app.h
GD/APP/waveform_analyzer_app.c
GD/APP/signal_generator_demo.h
GD/APP/signal_generator_demo.c
```

### 头文件包含
在主头文件中已添加：
```c
#include "dac_app.h"
#include "waveform_analyzer_app.h"
```

### 依赖库
- 标准数学库 (math.h)
- GD32F4xx标准外设库
- 现有的BSP层函数

## 7. 注意事项

1. **内存使用**: FFT分析需要较多RAM，注意内存分配
2. **时钟配置**: 确保系统时钟配置正确，影响定时器精度
3. **中断优先级**: DMA中断优先级需要合理配置
4. **采样率限制**: 受MCU性能限制，最高采样率约100kHz
5. **精度考虑**: 12位ADC/DAC精度，注意量化误差

## 8. 扩展建议

1. 添加更多波形类型（锯齿波、噪声等）
2. 实现更复杂的滤波算法
3. 添加频谱显示功能
4. 支持多通道同步采集
5. 实现波形存储和回放功能

---

**版权声明**: 本代码基于HAL项目逻辑，使用GD32F4xx标准库实现，适用于GD32F470VET6微控制器。
