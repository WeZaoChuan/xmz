#include "mcu_cmic_gd32f470vet6.h"

FATFS fs;
FIL fdst;
uint16_t i = 0, count, result = 0;
UINT br, bw;

sd_card_info_struct sd_cardinfo;

BYTE buffer[128];
BYTE filebuffer[128];

// 数据类型枚举
//typedef enum {
//    DATA_TYPE_INT8,      // 8位有符号整数
//    DATA_TYPE_UINT8,     // 8位无符号整数
//    DATA_TYPE_INT16,     // 16位有符号整数
//    DATA_TYPE_UINT16,    // 16位无符号整数
//    DATA_TYPE_INT32,     // 32位有符号整数
//    DATA_TYPE_UINT32,    // 32位无符号整数
//    DATA_TYPE_FLOAT,     // 单精度浮点数
//    DATA_TYPE_DOUBLE,    // 双精度浮点数
//    DATA_TYPE_STRING,    // 字符串数据
//    DATA_TYPE_BINARY     // 二进制数据（默认格式）
//} data_type_t;

ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        if(*src++ != *dst++)
            return ERROR;
    }
    return SUCCESS;
}

void sd_fatfs_init(void)
{
    nvic_irq_enable(SDIO_IRQn, 0, 0);        // 使能SDIO中断，优先级为0
}

/**
 * @brief       通过串口打印SD卡相关信息
 * @param       无
 * @retval      无
 */
void card_info_get(void)
{
    sd_card_info_struct sd_cardinfo;         // SD卡信息结构体
    sd_error_enum status;                    // SD卡操作状态
    uint32_t block_count, block_size;
    
    // 获取SD卡信息
    status = sd_card_information_get(&sd_cardinfo);
    
    if(SD_OK == status)
    {
        my_printf(DEBUG_USART, "\r\n*** SD Card Info ***\r\n");
        
        // 打印卡类型
        switch(sd_cardinfo.card_type)
        {
            case SDIO_STD_CAPACITY_SD_CARD_V1_1:
                my_printf(DEBUG_USART, "Card Type: Standard Capacity SD Card V1.1\r\n");
                break;
            case SDIO_STD_CAPACITY_SD_CARD_V2_0:
                my_printf(DEBUG_USART, "Card Type: Standard Capacity SD Card V2.0\r\n");
                break;
            case SDIO_HIGH_CAPACITY_SD_CARD:
                my_printf(DEBUG_USART, "Card Type: High Capacity SD Card\r\n");
                break;
            case SDIO_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: Multimedia Card\r\n");
                break;
            case SDIO_HIGH_CAPACITY_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: High Capacity Multimedia Card\r\n");
                break;
            case SDIO_HIGH_SPEED_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: High Speed Multimedia Card\r\n");
                break;
            default:
                my_printf(DEBUG_USART, "Card Type: Unknown\r\n");
                break;
        }
        
        // 打印卡容量和块大小
        block_count = (sd_cardinfo.card_csd.c_size + 1) * 1024;
        block_size = 512;
        my_printf(DEBUG_USART,"\r\n## Device size is %dKB (%.2fGB)##", 
                 sd_card_capacity_get(), 
                 sd_card_capacity_get() / 1024.0f / 1024.0f);
        my_printf(DEBUG_USART,"\r\n## Block size is %dB ##", block_size);
        my_printf(DEBUG_USART,"\r\n## Block count is %d ##", block_count);
        
        // 打印制造商ID和产品名称
        my_printf(DEBUG_USART, "Manufacturer ID: 0x%X\r\n", sd_cardinfo.card_cid.mid);
        my_printf(DEBUG_USART, "OEM/Application ID: 0x%X\r\n", sd_cardinfo.card_cid.oid);
        
        // 打印产品名称 (PNM)
        uint8_t pnm[6];
        pnm[0] = (sd_cardinfo.card_cid.pnm0 >> 24) & 0xFF;
        pnm[1] = (sd_cardinfo.card_cid.pnm0 >> 16) & 0xFF;
        pnm[2] = (sd_cardinfo.card_cid.pnm0 >> 8) & 0xFF;
        pnm[3] = sd_cardinfo.card_cid.pnm0 & 0xFF;
        pnm[4] = sd_cardinfo.card_cid.pnm1 & 0xFF;
        pnm[5] = '\0';
        my_printf(DEBUG_USART, "Product Name: %s\r\n", pnm);
        
        // 打印产品版本和序列号
        my_printf(DEBUG_USART, "Product Revision: %d.%d\r\n", 
                 (sd_cardinfo.card_cid.prv >> 4) & 0x0F, 
                 sd_cardinfo.card_cid.prv & 0x0F);
        my_printf(DEBUG_USART, "Product Serial Number: 0x%08X\r\n", sd_cardinfo.card_cid.psn);
        
        // 打印CSD版本和其它CSD信息
        my_printf(DEBUG_USART, "CSD Version: %d.0\r\n", sd_cardinfo.card_csd.csd_struct + 1);
    }
    else
    {
        my_printf(DEBUG_USART, "\r\nFailed to get SD card information, error code: %d\r\n", status);
    }
}

/**
 * @brief 将任意类型数据存储到SD卡文件
 * @param filename: 文件名（带路径）
 * @param data: 要存储的数据指针
 * @param data_size: 数据大小（字节）
 * @param max_retry: 最大重试次数
 * @return uint8_t: 1表示成功，0表示失败
 */
uint8_t sd_fatfs_save_data(const char *filename, void *data, uint32_t data_size, uint8_t max_retry)
{
    FIL file;
    FRESULT res;
    UINT bytes_written;
    uint8_t retry = 0;
    
    do {
        // 尝试打开文件（创建或覆盖）
        res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        
        if (res != FR_OK) {
            my_printf(DEBUG_USART, "打开文件失败: %s (%d), 重试 %d/%d\r\n", 
                     filename, res, retry + 1, max_retry);
            delay_ms(100);
            continue;
        }
        
        // 写入数据
        res = f_write(&file, data, data_size, &bytes_written);
        
        // 检查写入结果
        if ((res != FR_OK) || (bytes_written != data_size)) {
            f_close(&file);
            my_printf(DEBUG_USART, "写入失败: %s, 大小 %d != %d (%d), 重试 %d/%d\r\n", 
                     filename, bytes_written, data_size, res, retry + 1, max_retry);
            delay_ms(100);
            continue;
        }
        
        // 关闭文件并返回成功
        f_close(&file);
        my_printf(DEBUG_USART, "数据保存成功: %s (%d 字节)\r\n", filename, data_size);
        return 1;
    } while (retry++ < max_retry);
    
    my_printf(DEBUG_USART, "数据保存失败: %s (最大重试)\r\n", filename);
    return 0;
}

/**
 * @brief 从SD卡文件读取任意类型数据
 * @param filename: 文件名（带路径）
 * @param data: 存储读取数据的指针
 * @param max_size: 最大可接收数据大小（字节）
 * @param max_retry: 最大重试次数
 * @return uint32_t: 实际读取的字节数，0表示失败
 */
uint32_t sd_fatfs_read_data(const char *filename, void *data, uint32_t max_size, uint8_t max_retry)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    uint8_t retry = 0;
    
    do {
        // 尝试打开文件（只读）
        res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
        
        if (res != FR_OK) {
            my_printf(DEBUG_USART, "打开文件失败: %s (%d), 重试 %d/%d\r\n", 
                     filename, res, retry + 1, max_retry);
            delay_ms(100);
            continue;
        }
        
        // 检查文件大小
        uint32_t file_size = f_size(&file);
        
        // 确保不会溢出缓冲区
        uint32_t read_size = (file_size > max_size) ? max_size : file_size;
        
        // 定位到文件开始位置
        res = f_lseek(&file, 0);
        if (res != FR_OK) {
            f_close(&file);
            my_printf(DEBUG_USART, "文件定位失败: %s (%d), 重试 %d/%d\r\n", 
                     filename, res, retry + 1, max_retry);
            delay_ms(100);
            continue;
        }
        
        // 读取数据
        res = f_read(&file, data, read_size, &bytes_read);
        
        // 检查读取结果
        if ((res != FR_OK) || (bytes_read != read_size)) {
            f_close(&file);
            my_printf(DEBUG_USART, "读取失败: %s, 预期 %d, 实际 %d (%d), 重试 %d/%d\r\n", 
                     filename, read_size, bytes_read, res, retry + 1, max_retry);
            delay_ms(100);
            continue;
        }
        
        // 关闭文件并返回成功
        f_close(&file);
        my_printf(DEBUG_USART, "数据读取成功: %s (%d/%d 字节)\r\n", 
                 filename, bytes_read, read_size);
        return bytes_read;
    } while (retry++ < max_retry);
    
    my_printf(DEBUG_USART, "数据读取失败: %s (最大重试)\r\n", filename);
    return 0;
}

/**
 * @brief 智能打印任意类型数据
 * @param data: 要打印的数据指针
 * @param data_size: 数据大小（字节）
 * @param type: 数据类型枚举
 */
void print_data(const void *data, uint32_t data_size, data_type_t type)
{
    if (data == NULL || data_size == 0) {
        my_printf(DEBUG_USART, "无效数据: 空指针或零大小\r\n");
        return;
    }
    
    // 打印数据基本信息
    my_printf(DEBUG_USART, "数据大小: %u 字节 | ", data_size);
    
    // 根据数据类型进行不同的格式化输出
    switch(type) {
        case DATA_TYPE_INT8:
            my_printf(DEBUG_USART, "类型: int8_t (8位有符号整数)\r\n");
            for (uint32_t i = 0; i < data_size; i++) {
                if (i % 16 == 0) my_printf(DEBUG_USART, "%04X: ", i);
                int8_t value = *((const int8_t*)data + i);
                my_printf(DEBUG_USART, "%4d ", value);
                if ((i + 1) % 16 == 0 || i == data_size - 1) my_printf(DEBUG_USART, "\r\n");
            }
            break;
            
        case DATA_TYPE_UINT8:
            my_printf(DEBUG_USART, "类型: uint8_t (8位无符号整数)\r\n");
            for (uint32_t i = 0; i < data_size; i++) {
                if (i % 16 == 0) my_printf(DEBUG_USART, "%04X: ", i);
                uint8_t value = *((const uint8_t*)data + i);
                my_printf(DEBUG_USART, "0x%02X ", value);
                if ((i + 1) % 16 == 0 || i == data_size - 1) my_printf(DEBUG_USART, "\r\n");
            }
            break;
            
        case DATA_TYPE_INT16:
            my_printf(DEBUG_USART, "类型: int16_t (16位有符号整数)\r\n");
            for (uint32_t i = 0; i < data_size / sizeof(int16_t); i++) {
                if (i % 8 == 0) my_printf(DEBUG_USART, "%04X: ", i * sizeof(int16_t));
                int16_t value = *((const int16_t*)data + i);
                my_printf(DEBUG_USART, "%6d ", value);
                if ((i + 1) % 8 == 0 || i == data_size / sizeof(int16_t) - 1) 
                    my_printf(DEBUG_USART, "\r\n");
            }
            break;
            
        case DATA_TYPE_UINT16:
            my_printf(DEBUG_USART, "类型: uint16_t (16位无符号整数)\r\n");
            for (uint32_t i = 0; i < data_size / sizeof(uint16_t); i++) {
                if (i % 8 == 0) my_printf(DEBUG_USART, "%04X: ", i * sizeof(uint16_t));
                uint16_t value = *((const uint16_t*)data + i);
                my_printf(DEBUG_USART, "%5u(0x%04X) ", value, value);
                if ((i + 1) % 8 == 0 || i == data_size / sizeof(uint16_t) - 1) 
                    my_printf(DEBUG_USART, "\r\n");
            }
            break;
            
        case DATA_TYPE_INT32:
            my_printf(DEBUG_USART, "类型: int32_t (32位有符号整数)\r\n");
            for (uint32_t i = 0; i < data_size / sizeof(int32_t); i++) {
                int32_t value = *((const int32_t*)data + i);
                my_printf(DEBUG_USART, "[%2d]: %11d (0x%08lX)\r\n", 
                         i, value, (uint32_t)value);
            }
            break;
            
        case DATA_TYPE_UINT32:
            my_printf(DEBUG_USART, "类型: uint32_t (32位无符号整数)\r\n");
            for (uint32_t i = 0; i < data_size / sizeof(uint32_t); i++) {
                uint32_t value = *((const uint32_t*)data + i);
                my_printf(DEBUG_USART, "[%2d]: %10lu (0x%08lX)\r\n", i, value, value);
            }
            break;
            
        case DATA_TYPE_FLOAT:
            my_printf(DEBUG_USART, "类型: float (32位单精度浮点数)\r\n");
            for (uint32_t i = 0; i < data_size / sizeof(float); i++) {
                float value = *((const float*)data + i);
                uint32_t int_value = *((const uint32_t*)data + i);
                my_printf(DEBUG_USART, "[%2d]: %12.6f (0x%08lX)\r\n", 
                         i, value, int_value);
            }
            break;
            
        case DATA_TYPE_DOUBLE:
            my_printf(DEBUG_USART, "类型: double (64位双精度浮点数)\r\n");
            for (uint32_t i = 0; i < data_size / sizeof(double); i++) {
                double value = *((const double*)data + i);
                uint64_t int_value = *((const uint64_t*)data + i);
                my_printf(DEBUG_USART, "[%2d]: %14.9f (0x%016llX)\r\n", 
                         i, value, (unsigned long long)int_value);
            }
            break;
            
        case DATA_TYPE_STRING:
            my_printf(DEBUG_USART, "类型: 字符串 (ASCII文本)\r\n");
            my_printf(DEBUG_USART, "内容: \"%s\"\r\n", (const char*)data);
            my_printf(DEBUG_USART, "长度: %u 字节 (含终止符)\r\n", strlen((const char*)data) + 1);
            break;
            
        case DATA_TYPE_BINARY:
        default:
            my_printf(DEBUG_USART, "类型: 二进制数据\r\n");
            my_printf(DEBUG_USART, "十六进制转储:\r\n");
            uint32_t bytes_per_line = 16;
            for (uint32_t i = 0; i < data_size; i++) {
                if (i % bytes_per_line == 0) my_printf(DEBUG_USART, "%08X: ", i);
                my_printf(DEBUG_USART, "%02X ", ((const uint8_t*)data)[i]);
                if ((i + 1) % bytes_per_line == 0 || i == data_size - 1) {
                    // 补齐空位
                    if ((i % bytes_per_line) != 0 && i != data_size - 1) {
                        for (uint32_t j = (i % bytes_per_line); j < bytes_per_line; j++) {
                            my_printf(DEBUG_USART, "   ");
                        }
                    }
                    
                    // 打印ASCII表示
                    my_printf(DEBUG_USART, "  ");
                    uint32_t line_start = (i / bytes_per_line) * bytes_per_line;
                    uint32_t line_end = (line_start + bytes_per_line < data_size) ? 
                                      line_start + bytes_per_line : data_size;
                    for (uint32_t j = line_start; j < line_end; j++) {
                        uint8_t c = ((const uint8_t*)data)[j];
                        my_printf(DEBUG_USART, "%c", (c >= 0x20 && c <= 0x7E) ? c : '.');
                    }
                    my_printf(DEBUG_USART, "\r\n");
                }
            }
            break;
    }
}

void sd_fatfs_test(void)
{
    uint16_t k = 5;
    DSTATUS stat = 0;
    do
    {
        stat = disk_initialize(0); // 初始化SD卡
    } while((stat != 0) && (--k));
    
    card_info_get();
    
    my_printf(DEBUG_USART, "SD Card disk_initialize:%d\r\n", stat);
    f_mount(0, &fs); // 挂载文件系统
    my_printf(DEBUG_USART, "SD Card f_mount:%d\r\n", stat);
    
    if(RES_OK == stat)
    {        
        my_printf(DEBUG_USART, "\r\nSD卡初始化成功!\r\n");
        
        /****************** 测试各种数据类型的存储和读取 *******************/
        
        // 1. 测试整数变量
        int32_t number = 123456789;
        const char *number_file = "0:/DATA/number.bin";
        my_printf(DEBUG_USART, "\r\n测试整数变量: %ld\r\n", (long)number);
        if (sd_fatfs_save_data(number_file, &number, sizeof(number), 3)) {
            int32_t read_number;
            uint32_t bytes_read = sd_fatfs_read_data(number_file, &read_number, sizeof(read_number), 3);
            if (bytes_read == sizeof(number)) {
                //my_printf(DEBUG_USART, "读取结果: %ld\r\n", (long)read_number);
                print_data(&read_number, sizeof(read_number), DATA_TYPE_INT32);
            }
        }
        
        // 2. 测试浮点数变量
        float pi = 3.14159f;
        const char *pi_file = "0:/DATA/pi.bin";
        my_printf(DEBUG_USART, "\r\n测试浮点数变量: %f\r\n", pi);
        if (sd_fatfs_save_data(pi_file, &pi, sizeof(pi), 3)) {
            float read_pi;
            uint32_t bytes_read = sd_fatfs_read_data(pi_file, &read_pi, sizeof(read_pi), 3);
            if (bytes_read == sizeof(pi)) {
                //my_printf(DEBUG_USART, "读取结果: %f\r\n", read_pi);
                print_data(&read_pi, sizeof(read_pi), DATA_TYPE_FLOAT);
            }
        }
        
        // 3. 测试整数数组
        uint16_t array[5] = {10, 20, 30, 40, 50};
        const char *array_file = "0:/DATA/array.bin";
        uint32_t array_size = sizeof(array);
        my_printf(DEBUG_USART, "\r\n测试整数数组:");
        for (int i = 0; i < 5; i++) {
            my_printf(DEBUG_USART, " %u", array[i]);
        }
        my_printf(DEBUG_USART, "\r\n");
        if (sd_fatfs_save_data(array_file, array, array_size, 3)) {
            uint16_t read_array[5];
            uint32_t bytes_read = sd_fatfs_read_data(array_file, read_array, array_size, 3);
            if (bytes_read == array_size) {
//                my_printf(DEBUG_USART, "读取结果: ");
//                for (int i = 0; i < 5; i++) {
//                    my_printf(DEBUG_USART, "%u ", read_array[i]);
//                }
//                my_printf(DEBUG_USART, "\r\n");
                print_data(read_array, sizeof(read_array), DATA_TYPE_UINT16);
            }
        }
        
        // 4. 测试自定义结构体
        typedef struct {
            uint32_t id;
            char name[12];
            float temperature;
            uint8_t status;
        } SensorData;
        
        SensorData sensor = {
            .id = 1234,
            .name = "Sensor-01",
            .temperature = 25.4f,
            .status = 0xA5
        };
        
        const char *sensor_file = "0:/DATA/sensor.bin";
        uint32_t sensor_size = sizeof(sensor);
        my_printf(DEBUG_USART, "\r\n测试传感器结构体: ID=%lu, 名称=%s, 温度=%.1f, 状态=0x%02X\r\n",
                 sensor.id, sensor.name, sensor.temperature, sensor.status);
        if (sd_fatfs_save_data(sensor_file, &sensor, sensor_size, 3)) {
            SensorData read_sensor;
            uint32_t bytes_read = sd_fatfs_read_data(sensor_file, &read_sensor, sensor_size, 3);
            if (bytes_read == sensor_size) {
                my_printf(DEBUG_USART, "读取结果: ID=%lu, 名称=%s, 温度=%.1f, 状态=0x%02X\r\n",
                         read_sensor.id, read_sensor.name, read_sensor.temperature, read_sensor.status);
               print_data(&read_sensor, sizeof(read_sensor), DATA_TYPE_BINARY);
            }
        }
        
        // 5. 测试文本文件
        const char *text = "Hello, MCUSTUDIO! This is a text file.";
        uint32_t text_size = strlen(text) + 1; // 包括终止符
        const char *text_file = "0:/DATA/text.bin";
        my_printf(DEBUG_USART, "\r\n测试文本文件: \"%s\"\r\n", text);
        
        if (sd_fatfs_save_data(text_file, (void*)text, text_size, 3)) {
            char read_text[128] = {0};
            uint32_t bytes_read = sd_fatfs_read_data(text_file, read_text, sizeof(read_text), 3);
            if (bytes_read > 0) {
                //my_printf(DEBUG_USART, "读取结果: %s\r\n", read_text);
                print_data(read_text, bytes_read, DATA_TYPE_STRING);
            }
        }
    }
    
    // 卸载文件系统
    f_mount(0, NULL);
    my_printf(DEBUG_USART, "\r\nSD卡文件系统卸载完成\r\n");
}



