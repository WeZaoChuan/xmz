#include "mcu_cmic_gd32f470vet6.h"

extern uint16_t adc_value[1];
extern uint16_t convertarr[CONVERT_NUM];

// 全局变量定义
uint8_t adc_recording_flag = 0; // ADC数据采集状态标志：0-停止，1-采集中
char adc_filename[32] = {0}; // ADC数据文件名
static FIL adc_file; // ADC数据文件句柄
static uint32_t last_adc_time = 0; // 上次ADC采集时间

void adc_task(void)
{
    convertarr[0] = adc_value[0];

    // 如果正在记录且到达采集间隔
    if (adc_recording_flag && (get_system_ms() - last_adc_time >= 100)) {
        last_adc_time = get_system_ms();

        // 准备数据字符串：ADC数据-时间戳
        char data_buffer[64];
        uint32_t current_time = get_system_ms();
        sprintf(data_buffer, "%d-%lu\r\n", adc_value[0], current_time);

        // 写入文件
        UINT bytes_written;
        FRESULT result = f_write(&adc_file, data_buffer, strlen(data_buffer), &bytes_written);
        if (result == FR_OK) {
            f_sync(&adc_file); // 立即同步到存储设备
        } else {
            // 写入失败，停止记录
            my_printf(DEBUG_USART, "ADC write error: %d, stopping recording\r\n", result);
            adc_stop_recording();
        }
    }
}

void adc_start_recording(void)
{
    if (!adc_recording_flag) {
        // 确保之前的文件已关闭
        f_close(&adc_file);

        // 检查文件系统状态
        DWORD free_clusters;
        FATFS *fs;
        FRESULT fs_result = f_getfree("0:", &free_clusters, &fs);
        if (fs_result != FR_OK) {
            my_printf(DEBUG_USART, "File system error: %d, trying to remount...\r\n", fs_result);
            // 尝试重新挂载文件系统
            f_mount(0, NULL); // 卸载
            delay_ms(100);
            extern FATFS fs; // 使用外部定义的文件系统对象
            f_mount(0, &fs); // 重新挂载
            delay_ms(100);
        } else {
            my_printf(DEBUG_USART, "File system OK, free clusters: %lu\r\n", free_clusters);
        }

        // 生成文件名：按下按键时的时间戳
        uint32_t timestamp = get_system_ms();
        sprintf(adc_filename, "0:/ADC_%lu.txt", timestamp);

        // 检查文件是否已存在
        FIL test_file;
        FRESULT test_result = f_open(&test_file, adc_filename, FA_READ);
        if (test_result == FR_OK) {
            f_close(&test_file);
            my_printf(DEBUG_USART, "File exists, deleting: %s\r\n", adc_filename);
            f_unlink(adc_filename); // 删除已存在的文件
        }

        // 创建并打开文件
        FRESULT result = f_open(&adc_file, adc_filename, FA_CREATE_ALWAYS | FA_WRITE);
        if (result == FR_OK) {
            adc_recording_flag = 1;
            last_adc_time = get_system_ms();
            my_printf(DEBUG_USART, "ADC recording started: %s\r\n", adc_filename);
        } else {
            my_printf(DEBUG_USART, "Failed to create ADC file: %s (Error: %d)\r\n", adc_filename, result);
            // 尝试使用简化文件名
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
        f_sync(&adc_file); // 确保数据写入存储设备
        f_close(&adc_file); // 关闭文件
        adc_recording_flag = 0;
        my_printf(DEBUG_USART, "ADC recording stopped: %s\r\n", adc_filename); // 调试信息
    } else {
        my_printf(DEBUG_USART, "No ADC recording in progress\r\n"); // 状态提示
    }
}

void adc_recording_init(void)
{
    // 初始化全局变量
    adc_recording_flag = 0;
    memset(adc_filename, 0, sizeof(adc_filename));
    last_adc_time = 0;

    // 确保文件句柄处于关闭状态
    f_close(&adc_file);

    // 检查文件系统状态
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

        // 尝试重新初始化文件系统
        extern FATFS fs; // 使用sd_app.c中定义的fs
        my_printf(DEBUG_USART, "Attempting to reinitialize file system...\r\n");
        f_mount(0, NULL); // 卸载
        delay_ms(200);
        result = f_mount(0, &fs); // 重新挂载
        if (result == FR_OK) {
            my_printf(DEBUG_USART, "File system reinitialized successfully\r\n");
        } else {
            my_printf(DEBUG_USART, "File system reinitialize failed: %d\r\n", result);
        }
    }
}

