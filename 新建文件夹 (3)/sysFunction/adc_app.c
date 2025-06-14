#include "adc_app.h"
#include "tim.h"
#include "dac_app.h"
#include "sd_fatfs.h"

// 1 轮询
// 2 DMA连续转换
// 3 DMA TIM 多通道采集
#define ADC_MODE (3)

// --- 保留 ADC 相关代码 ---

#if ADC_MODE == 1

__IO uint32_t adc_val; // 用于存储计算后的平均 ADC 值
__IO float voltage;    // 用于存储计算后的电压值

// 在需要读取 ADC 的地方调用，比如一个任务函数内
void adc_task(void)
{
    // 1. 启动 ADC 转换
    HAL_ADC_Start(&hadc1); // hadc1 是你的 ADC 句柄

    // 2. 等待转换完成 (阻塞式)
    //    参数 1000 表示超时时间 (毫秒)
    if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK)
    {
        // 3. 转换成功，读取数字结果 (0-4095 for 12-bit)
        adc_val = HAL_ADC_GetValue(&hadc1);

        // 4. (可选) 将数字值转换为实际电压值
        //    假设 Vref = 3.3V, 分辨率 12 位 (4096)
        voltage = (float)adc_val * 3.3f / 4096.0f;

        // (这里可以加入你对 voltage 或 adc_val 的处理逻辑)
        my_printf(&huart1, "ADC Value: %lu, Voltage: %.2fV\n", adc_val, voltage);
    }
    else
    {
        // 转换超时或出错处理
        // my_printf(&huart1, "ADC Poll Timeout!\n");
    }

    // 5. （重要）如果 ADC 配置为单次转换模式，通常不需要手动停止。
    //    如果是连续转换模式，可能需要 HAL_ADC_Stop(&hadc1);
    // HAL_ADC_Stop(&hadc1); // 根据你的 CubeMX 配置决定是否需要
}

#elif ADC_MODE == 2

// --- 全局变量 ---
#define ADC_DMA_BUFFER_SIZE 32 // DMA缓冲区大小，可以根据需要调整
uint32_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE]; // DMA 目标缓冲区
__IO uint32_t adc_val;                        // 用于存储计算后的平均 ADC 值
__IO float voltage;                           // 用于存储计算后的电压值

// --- 初始化 (通常在 main 函数或外设初始化函数中调用一次) ---
void adc_dma_init(void)
{
    // 启动 ADC 并使能 DMA 传输
    // hadc1: ADC 句柄
    // (uint32_t*)adc_dma_buffer: DMA 目标缓冲区地址 (HAL库通常需要uint32_t*)
    // ADC_DMA_BUFFER_SIZE: 本次传输的数据量 (缓冲区大小)

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_DMA_BUFFER_SIZE);
}

// --- 处理任务 (在主循环或定时器回调中定期调用) ---
void adc_task(void)
{
    uint32_t adc_sum = 0;

    // 1. 计算 DMA 缓冲区中所有采样值的总和
    //    注意：这里直接读取缓冲区，可能包含不同时刻的采样值
    for (uint16_t i = 0; i < ADC_DMA_BUFFER_SIZE; i++)
    {
        adc_sum += adc_dma_buffer[i];
    }

    // 2. 计算平均 ADC 值
    adc_val = adc_sum / ADC_DMA_BUFFER_SIZE;

    // 3. (可选) 将平均数字值转换为实际电压值
    voltage = ((float)adc_val * 3.3f) / 4096.0f; // 假设12位分辨率, 3.3V参考电压

    // 4. 使用计算出的平均值 (adc_val 或 voltage)
    my_printf(&huart1, "Average ADC: %lu, Voltage: %.2fV\n", adc_val, voltage);
}

#elif ADC_MODE == 3

#define BUFFER_SIZE 2048

extern DMA_HandleTypeDef hdma_adc1;

uint32_t adc_value_buffer[BUFFER_SIZE / 2];
uint32_t dac_val_buffer[BUFFER_SIZE / 2];
uint32_t res_val_buffer[BUFFER_SIZE / 2];
__IO uint32_t adc_val_buffer[BUFFER_SIZE];
__IO float voltage;
__IO uint8_t AdcConvEnd = 0;
uint8_t wave_analysis_flag = 0; // 波形分析标志位
uint8_t wave_query_type = 0;    // 波形查询类型：0=全部, 1=类型, 2=频率, 3=峰峰值

// ADC数据记录相关变量
volatile uint8_t adc_record_flag = 0;     // 数据记录标志
volatile uint8_t adc_print_flag = 0;      // 数据打印标志
AdcDataRecord adc_data_buffer[MAX_ADC_RECORDS];  // 数据缓冲区
volatile uint16_t adc_record_count = 0;   // 当前记录数量
static uint8_t adc_record_counter = 0;    // 记录间隔计数器，用于10ms间隔控制

void adc_tim_dma_init(void)
{
    HAL_TIM_Base_Start(&htim3);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT); // 禁用半传输中断
}

// ADC 转换完成回调
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    UNUSED(hadc);
    if (hadc == &hadc1) // 确保是 hadc1 完成
    {
        HAL_ADC_Stop_DMA(hadc); // 停止 DMA，等待处理
        AdcConvEnd = 1;         // 设置标志位
    }
}

// 在usart_app.c中声明为extern
WaveformInfo wave_data;

void adc_task(void)
{
    // 一次数据转换 3(采样) + 12.5(转换) = 15.5 ADC时钟周期
    // 假设 ADC 时钟 14MHz (来自 HSI/PLL), 一次转换时间: 15.5 / 14MHz ~= 1.1 us
    // BUFFER_SIZE 次转换总时间: 1000 * 1.1 us = 1.1 ms (估算值)
    // 定时器触发频率需要匹配这个速率或更慢
    // 当前调度器设置为1ms执行一次，数据记录改为10ms间隔

    // 每10ms记录一次ADC数据，不依赖DMA完成标志
    if (adc_record_flag && adc_record_count < MAX_ADC_RECORDS)
    {
			  uint32_t   adc_sum=0;
        adc_record_counter++;
        if (adc_record_counter >= 10) // 每10个1ms周期记录一次，即10ms间隔
        {
           adc_record_counter = 0; // 重置计数器
					 for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
          {
           adc_value_buffer[i] = adc_val_buffer[i * 2 ]; // 将 ADC 数据存入名为 dac_val_buffer 的数
          }
				  for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
          {
           adc_sum += res_val_buffer[i];
          }
          uint32_t current_sample = adc_sum / (BUFFER_SIZE / 2); // 取第一个样本，保持数据的实时变化
          float current_voltage = (float)current_sample * 3.3f / 4096.0f;

          // 记录数据
          adc_data_buffer[adc_record_count].timestamp = HAL_GetTick();
          adc_data_buffer[adc_record_count].adc_value = current_sample;
          adc_data_buffer[adc_record_count].voltage = current_voltage;
          adc_record_count++;
        }

            // 如果缓冲区满了，自动停止记录并保存
            if (adc_record_count >= MAX_ADC_RECORDS)
            {
                adc_record_flag = 0;
                my_printf(&huart1, "ADC数据记录已满，共记录%d条数据，正在保存到SD卡...\r\n", adc_record_count);
                adc_save_data_to_sd();
            }
        }

    if (AdcConvEnd) // 检查转换完成标志
    {
        // --- 处理采集到的数据 ---
        // 示例：将奇数索引的数据复制到另一个缓冲区 
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            dac_val_buffer[i] = adc_val_buffer[i * 2 + 1]; // 将 ADC 数据存入名为 dac_val_buffer 的数组
            res_val_buffer[i] = adc_val_buffer[i * 2];
        }
        uint32_t res_sum = 0;
        // 将 res_val_buffer 中的数据转换为电压值
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            res_sum += res_val_buffer[i];
        }
				
        uint32_t res_avg = res_sum / (BUFFER_SIZE / 2);
        voltage = (float)res_avg * 3.3f / 4096.0f;
        // 将 voltage (0-3.3V) 映射到 DAC 峰值幅度 (0-1650mV，假设 VREF=3.3V)
        dac_app_set_amplitude((uint16_t)(voltage * 500.0f));

        // 注意：ADC数据记录逻辑已移到adc_task开头，每10ms执行一次

        if (uart_send_flag == 1)
        {
            // 示例：通过串口打印处理后的数据
            for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
            {
                // 注意: 原代码格式化字符串 "{dac}%d\r\n" 可能用于特定解析，予以保留
                my_printf(&huart1, "{dac}%d\r\n", (int)dac_val_buffer[i]);
            }
        }

        // 如果设置了波形分析标志，则进行波形分析
        if (wave_analysis_flag)
        {
            wave_data = Get_Waveform_Info(dac_val_buffer);

            // 根据查询类型打印相应的信息
            switch (wave_query_type)
            {
            case 4: // 打印全部信息
                my_printf(&huart1, "输入频率: %d FFT频率：%d 峰峰值：%.2f 波形类型：%s\r\n",
                          dac_app_get_update_frequency() / WAVEFORM_SAMPLES,
                          (uint32_t)wave_data.frequency,
                          wave_data.vpp,
                          GetWaveformTypeString(wave_data.waveform_type));
                break;

            case 1: // 仅打印波形类型
                my_printf(&huart1, "当前波形类型: %s\r\n", GetWaveformTypeString(wave_data.waveform_type));
                break;

            case 2: // 仅打印频率
                my_printf(&huart1, "当前频率: %dHz\r\n", (uint32_t)wave_data.frequency);
                break;

            case 3: // 仅打印峰峰值
                my_printf(&huart1, "当前峰峰值: %.2fmV\r\n", wave_data.vpp);
                break;
            }

            wave_analysis_flag = 0; // 分析完成后清除标志
            wave_query_type = 0;    // 重置查询类型
        }

        // --- 处理完成 ---

        // 清空处理缓冲区 (可选，取决于后续逻辑)
        // memset(dac_val_buffer, 0, sizeof(uint32_t) * (BUFFER_SIZE / 2));

        // 清空 ADC DMA 缓冲区和标志位，准备下一次采集
        // memset(adc_val_buffer, 0, sizeof(uint32_t) * BUFFER_SIZE); // 清空原始数据 (如果需要)
        AdcConvEnd = 0;

        // 重新启动 ADC DMA 进行下一次采集
        // 注意：如果定时器是连续触发 ADC 的，可能不需要手动停止/启动 DMA
        // 需要根据 TIM3 和 ADC 的具体配置决定是否需要重新启动
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
        __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT); // 再次禁用半传输中断
    }
}


// ADC数据记录控制函数
void adc_start_recording(void)
{
    adc_record_count = 0;    // 重置记录计数器
    adc_record_counter = 0;  // 重置间隔计数器
    adc_record_flag = 1;     // 开始记录
    my_printf(&huart1, "开始ADC数据记录（10ms间隔）...\r\n");
}

void adc_stop_recording(void)
{
    adc_record_flag = 0;   // 停止记录
    my_printf(&huart1, "停止ADC数据记录，共记录%d条数据\r\n", adc_record_count);
}

void adc_save_data_to_sd(void)
{
    if (adc_record_count == 0)
    {
        my_printf(&huart1, "没有数据需要保存\r\n");
        return;
    }

    // 计算总的数据大小
    uint32_t total_size = adc_record_count * sizeof(AdcDataRecord);

    // 保存到SD卡
    if (sd_fatfs_write_data("DATA/adc_records.dat", DATA_TYPE_STRUCT, adc_data_buffer, total_size))
    {
        my_printf(&huart1, "ADC数据已保存到SD卡，共%d条记录\r\n", adc_record_count);
    }
    else
    {
        my_printf(&huart1, "ADC数据保存失败\r\n");
    }
}

void adc_print_data_from_sd(void)
{
    // 首先尝试读取数据
    AdcDataRecord temp_buffer[MAX_ADC_RECORDS];
    uint32_t bytes_read;

    // 读取文件获取实际大小
    if (!sd_fatfs_read_file("DATA/adc_records.dat", temp_buffer, sizeof(temp_buffer), &bytes_read))
    {
        my_printf(&huart1, "读取ADC数据文件失败\r\n");
        return;
    }

    // 计算记录数量
    uint32_t record_count = bytes_read / sizeof(AdcDataRecord);

    if (record_count == 0)
    {
        my_printf(&huart1, "没有找到ADC数据记录\r\n");
        return;
    }

    my_printf(&huart1, "=== ADC数据记录 (共%lu条) ===\r\n", record_count);

    // 逐条打印数据
    for (uint32_t i = 0; i < record_count; i++)
    {
        my_printf(&huart1, "时间戳：%lu，ADC值：%lu，电压值：%f\r\n",
                  temp_buffer[i].timestamp, temp_buffer[i].adc_value,temp_buffer[i].voltage);
    }

    my_printf(&huart1, "=== 数据打印完成 ===\r\n");
}

#endif
