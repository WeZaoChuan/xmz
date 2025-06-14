#ifndef __SD_FATFS_H_
#define __SD_FATFS_H_


#include "ff.h"
#include "diskio.h"
#include "mydefine.h"

// 定义传感器数据结构体
typedef struct {
    uint32_t timestamp;  // 时间戳
    float voltage;       // 电压值
    float current;       // 电流值
    uint8_t status;      // 状态标志
}SensorData;

// 定义测试用的复杂结构体
typedef struct {
    uint32_t id;
    char name[32];
    float values[4];
    uint8_t flags;
} TestStruct;

// 数据类型枚举
typedef enum {
    DATA_TYPE_INT8 = 1,
    DATA_TYPE_INT16,
    DATA_TYPE_INT32,
    DATA_TYPE_UINT8,
    DATA_TYPE_UINT16,
    DATA_TYPE_UINT32,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_STRING,
    DATA_TYPE_STRUCT
} DataType;

// 基础函数声明
uint8_t sd_fatfs_init(void);
void sd_fatfs_deinit(void);

// 基本文件操作函数声明
uint8_t sd_fatfs_read_file(const char* path, void* buffer, uint32_t size, uint32_t* bytes_read);
uint8_t sd_fatfs_write_file(const char* path, const void* buffer, uint32_t size, uint32_t* bytes_written);
uint8_t sd_fatfs_print_file(const char* path);

// 统一的数据写入函数
uint8_t sd_fatfs_write_data(const char* path, DataType type, const void* data, uint32_t count);

// 统一的数据读取函数
uint8_t sd_fatfs_read_data(const char* path, DataType type, void* data, uint32_t count, uint32_t max_size);

// 统一的数据打印函数
uint8_t sd_fatfs_print_data(const char* path, DataType type, uint32_t count);

// 测试函数声明
void sd_fatfs_test_all_data_types(void);

#endif /* __SD_FATFS_H */

