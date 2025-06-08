#include "mcu_cmic_gd32f470vet6.h"
#include "adc_app.h"
#include <string.h>

extern uint16_t adc_value[1];
extern uint16_t convertarr[CONVERT_NUM];

// ??????????
uint8_t adc_recording_flag = 0; // ADC?????????????0-????1-?????
char adc_filename[32] = {0}; // ADC?????????
static FIL adc_file; // ADC??????????
static uint32_t last_adc_time = 0; // ???ADC??????

void adc_task(void)
{
    convertarr[0] = adc_value[0];

    // ???????????????????
    if (adc_recording_flag && (get_system_ms() - last_adc_time >= 100)) {
        last_adc_time = get_system_ms();

        // ??????????????ADC????-????
        char data_buffer[64];
        uint32_t current_time = get_system_ms();
        sprintf(data_buffer, "%d-%lu\r\n", adc_value[0], current_time);

        // д?????
        UINT bytes_written;
        FRESULT result = f_write(&adc_file, data_buffer, strlen(data_buffer), &bytes_written);
        if (result == FR_OK) {
            f_sync(&adc_file); // ??????????洢?豸
        } else {
            // д???????????
            my_printf(DEBUG_USART, "ADC write error: %d, stopping recording\r\n", result);
            adc_stop_recording();
        }
    }
}

void adc_start_recording(void)
{
    if (!adc_recording_flag) {
        // ??????????????
        f_close(&adc_file);

        // ??????????
        DWORD free_clusters;
        FATFS *fs;
        FRESULT fs_result = f_getfree("0:", &free_clusters, &fs);
        if (fs_result != FR_OK) {
            my_printf(DEBUG_USART, "File system error: %d, trying to remount...\r\n", fs_result);
            // ????????????????
            f_mount(0, NULL); // ж??
            delay_ms(100);
            extern FATFS fs; // ???????????????????
            f_mount(0, &fs); // ???????
            delay_ms(100);
        } else {
            my_printf(DEBUG_USART, "File system OK, free clusters: %lu\r\n", free_clusters);
        }

        // ??????????????°??????????
        uint32_t timestamp = get_system_ms();
        sprintf(adc_filename, "0:/ADC_%lu.txt", timestamp);

        // ??????????????
        FIL test_file;
        FRESULT test_result = f_open(&test_file, adc_filename, FA_READ);
        if (test_result == FR_OK) {
            f_close(&test_file);
            my_printf(DEBUG_USART, "File exists, deleting: %s\r\n", adc_filename);
            f_unlink(adc_filename); // ????????????
        }

        // ???????????
        FRESULT result = f_open(&adc_file, adc_filename, FA_CREATE_ALWAYS | FA_WRITE);
        if (result == FR_OK) {
            adc_recording_flag = 1;
            last_adc_time = get_system_ms();
            my_printf(DEBUG_USART, "ADC recording started: %s\r\n", adc_filename);
        } else {
            my_printf(DEBUG_USART, "Failed to create ADC file: %s (Error: %d)\r\n", adc_filename, result);
            // ??????ü??????
            sprintf(adc_filename, "0:/ADC.txt");
            result = f_open(&adc_file, adc_filename, FA_CREATE_ALWAYS | FA_WRITE);
            if (result == FR_OK) {
                adc_recording_flag = 1;
                last_adc_time = get_system_ms();
                my_printf(DEBUG_USART, "ADC recording started with fallback name: %s\r\n", adc_filename);
            } else {
                my_printf(DEBUG_USART, "Fallback file creation also failed (Error: %d)\r\n", result);
            }
        }
    } else {
        my_printf(DEBUG_USART, "ADC recording already in progress\r\n");
    }
}

void adc_stop_recording(void)
{
    if (adc_recording_flag) {
        f_sync(&adc_file); // ???????д??洢?豸
        f_close(&adc_file); // ??????
        adc_recording_flag = 0;
        my_printf(DEBUG_USART, "ADC recording stopped: %s\r\n", adc_filename); // ???????
    } else {
        my_printf(DEBUG_USART, "No ADC recording in progress\r\n"); // ?????
    }
}

void adc_recording_init(void)
{
    // ???????????
    adc_recording_flag = 0;
    memset(adc_filename, 0, sizeof(adc_filename));
    last_adc_time = 0;

    // ?????????????????
    f_close(&adc_file);

    // ??????????
    adc_check_filesystem();

    my_printf(DEBUG_USART, "ADC recording system initialized\r\n");
}

void adc_check_filesystem(void)
{
    DWORD free_clusters;
    FATFS *fs;
    FRESULT result = f_getfree("0:", &free_clusters, &fs);

    if (result == FR_OK) {
        my_printf(DEBUG_USART, "File system status: OK\r\n");
        my_printf(DEBUG_USART, "Free clusters: %lu\r\n", free_clusters);
        my_printf(DEBUG_USART, "Cluster size: %d bytes\r\n", fs->csize * 512);
        my_printf(DEBUG_USART, "Free space: %lu KB\r\n", (free_clusters * fs->csize) / 2);
    } else {
        my_printf(DEBUG_USART, "File system error: %d\r\n", result);

        // ?????????????????
        extern FATFS fs; // ???sd_app.c?ж????fs
        my_printf(DEBUG_USART, "Attempting to reinitialize file system...\r\n");
        f_mount(0, NULL); // ж??
        delay_ms(200);
        result = f_mount(0, &fs); // ???????
        if (result == FR_OK) {
            my_printf(DEBUG_USART, "File system reinitialized successfully\r\n");
        } else {
            my_printf(DEBUG_USART, "File system reinitialize failed: %d\r\n", result);
        }
    }
}

// --- 新增高级ADC功能变量 ---
uint16_t adc_buffer[ADC_BUFFER_SIZE];           // ADC数据缓冲区
volatile adc_state_t adc_current_state = ADC_STATE_IDLE;  // ADC当前状态
static adc_config_t adc_config;                 // ADC配置
static uint16_t adc_sample_count = 0;           // 当前采样计数
static uint16_t adc_target_samples = 0;         // 目标采样数
static uint16_t adc_buffer_index = 0;           // 缓冲区索引
static uint32_t adc_sample_rate_hz = 1000;     // 采样频率
static uint8_t adc_initialized = 0;             // 初始化标志

// --- 私有函数原型 ---
static int adc_config_timer(uint32_t freq_hz);
static int adc_config_dma(void);
static void adc_reset_buffer(void);

// --- 私有函数实现 ---

// 配置ADC定时器
static int adc_config_timer(uint32_t freq_hz)
{
    if (freq_hz == 0) return -1;

    // 计算定时器参数
    uint32_t timer_period = ADC_TIMER_CLOCK_HZ / freq_hz;
    if (timer_period == 0) timer_period = 1;
    if (timer_period > 65535) timer_period = 65535;

    // 停止定时器
    timer_disable(TIMER0);

    // 重新配置定时器
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = 239;  // 预分频器
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (timer_period / 240) - 1;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(TIMER0, &timer_initpara);
    timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_UPDATE);

    return 0;
}

// 配置ADC DMA
static int adc_config_dma(void)
{
    // 停止DMA
    dma_channel_disable(DMA1, DMA_CH0);

    // 等待DMA停止
    while (dma_flag_get(DMA1, DMA_CH0, DMA_FLAG_FTF));

    // 重新配置DMA
    DMA_CHPADDR(DMA1, DMA_CH0) = (uint32_t)(&ADC_RDATA(ADC0));
    DMA_CHM0ADDR(DMA1, DMA_CH0) = (uint32_t)adc_buffer;
    DMA_CHCNT(DMA1, DMA_CH0) = ADC_BUFFER_SIZE;

    return 0;
}

// 重置缓冲区
static void adc_reset_buffer(void)
{
    adc_buffer_index = 0;
    adc_sample_count = 0;
    memset(adc_buffer, 0, sizeof(adc_buffer));
}

// --- 高级ADC功能实现 ---

int adc_app_init(const adc_config_t *config)
{
    if (!config) return -1;

    // 保存配置
    memcpy(&adc_config, config, sizeof(adc_config_t));
    adc_sample_rate_hz = config->sample_rate_hz;

    // 初始化硬件
    bsp_adc_init();

    // 配置定时器触发
    if (config->mode == ADC_MODE_TIMER_TRIGGER || config->mode == ADC_MODE_DMA_CIRCULAR)
    {
        if (adc_config_timer(config->sample_rate_hz) != 0) return -1;
    }

    // 配置DMA
    if (adc_config_dma() != 0) return -1;

    // 重置状态
    adc_current_state = ADC_STATE_IDLE;
    adc_reset_buffer();

    adc_initialized = 1;
    return 0;
}

int adc_app_start_sampling(uint16_t samples)
{
    if (!adc_initialized) return -1;
    if (adc_current_state != ADC_STATE_IDLE) return -1;

    adc_target_samples = samples;
    adc_reset_buffer();
    adc_current_state = ADC_STATE_SAMPLING;

    // 根据模式启动采样
    switch (adc_config.mode)
    {
    case ADC_MODE_SINGLE:
        adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
        break;

    case ADC_MODE_CONTINUOUS:
        adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
        adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
        break;

    case ADC_MODE_TIMER_TRIGGER:
        adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_RISING);
        timer_enable(TIMER0);
        break;

    case ADC_MODE_DMA_CIRCULAR:
        dma_circulation_enable(DMA1, DMA_CH0);
        dma_channel_enable(DMA1, DMA_CH0);
        adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_RISING);
        timer_enable(TIMER0);
        break;
    }

    return 0;
}

int adc_app_stop_sampling(void)
{
    if (!adc_initialized) return -1;

    // 停止定时器
    timer_disable(TIMER0);

    // 停止DMA
    dma_channel_disable(DMA1, DMA_CH0);

    // 停止ADC
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);

    adc_current_state = ADC_STATE_IDLE;
    return 0;
}

int adc_app_set_sample_rate(uint32_t sample_rate_hz)
{
    if (!adc_initialized) return -1;
    if (adc_current_state != ADC_STATE_IDLE) return -1;

    adc_sample_rate_hz = sample_rate_hz;
    adc_config.sample_rate_hz = sample_rate_hz;

    return adc_config_timer(sample_rate_hz);
}

uint32_t adc_app_get_sample_rate(void)
{
    return adc_sample_rate_hz;
}

int adc_app_config_timer_trigger(uint32_t timer_freq_hz)
{
    if (!adc_initialized) return -1;

    return adc_config_timer(timer_freq_hz);
}

adc_state_t adc_app_get_state(void)
{
    return adc_current_state;
}

uint16_t adc_app_get_data(uint16_t *buffer, uint16_t size)
{
    if (!buffer || size == 0) return 0;

    uint16_t copy_size = (size < adc_sample_count) ? size : adc_sample_count;
    memcpy(buffer, adc_buffer, copy_size * sizeof(uint16_t));

    return copy_size;
}

uint16_t adc_app_raw_to_mv(uint16_t raw_value)
{
    return (uint16_t)((uint32_t)raw_value * ADC_VREF_MV / ADC_MAX_VALUE);
}

uint16_t adc_app_mv_to_raw(uint16_t voltage_mv)
{
    return (uint16_t)((uint32_t)voltage_mv * ADC_MAX_VALUE / ADC_VREF_MV);
}

int adc_app_get_statistics(const uint16_t *buffer, uint16_t size,
                          uint16_t *min, uint16_t *max, float *mean)
{
    if (!buffer || size == 0 || !min || !max || !mean) return -1;

    uint16_t min_val = buffer[0];
    uint16_t max_val = buffer[0];
    uint32_t sum = 0;

    for (uint16_t i = 0; i < size; i++)
    {
        if (buffer[i] < min_val) min_val = buffer[i];
        if (buffer[i] > max_val) max_val = buffer[i];
        sum += buffer[i];
    }

    *min = min_val;
    *max = max_val;
    *mean = (float)sum / size;

    return 0;
}

void adc_app_dma_complete_callback(void)
{
    if (adc_current_state == ADC_STATE_SAMPLING)
    {
        adc_sample_count = ADC_BUFFER_SIZE;
        adc_current_state = ADC_STATE_COMPLETE;

        // 如果是单次采样模式，停止采样
        if (adc_config.mode != ADC_MODE_DMA_CIRCULAR)
        {
            adc_app_stop_sampling();
        }
    }
}

void adc_app_dma_half_complete_callback(void)
{
    if (adc_current_state == ADC_STATE_SAMPLING)
    {
        adc_sample_count = ADC_BUFFER_SIZE / 2;
    }
}

