#ifndef __SD_APP_H_
#define __SD_APP_H_

#include "stdint.h"
typedef enum {
    DATA_TYPE_INT8,      // 8位有符号整数
    DATA_TYPE_UINT8,     // 8位无符号整数
    DATA_TYPE_INT16,     // 16位有符号整数
    DATA_TYPE_UINT16,    // 16位无符号整数
    DATA_TYPE_INT32,     // 32位有符号整数
    DATA_TYPE_UINT32,    // 32位无符号整数
    DATA_TYPE_FLOAT,     // 单精度浮点数
    DATA_TYPE_DOUBLE,    // 双精度浮点数
    DATA_TYPE_STRING,    // 字符串数据
    DATA_TYPE_BINARY     // 二进制数据（默认格式）
} data_type_t;
void print_adc_file_info(void);
void sd_fatfs_init(void);
void sd_fatfs_test(void);
uint8_t sd_fatfs_save_data(const char *filename, void *data, uint32_t data_size, uint8_t max_retry);
uint32_t sd_fatfs_read_data(const char *filename, void *data, uint32_t max_size, uint8_t max_retry);
void print_data(const void *data, uint32_t data_size, data_type_t type);
#endif /* __SD_APP_H_ */
