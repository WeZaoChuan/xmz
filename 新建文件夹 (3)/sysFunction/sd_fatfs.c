#include "sd_fatfs.h"
#include "usart_app.h"
#include "string.h"
#include "fatfs.h"
#include "usart.h"
#include "stdlib.h"


//lfs_t lfs;
//struct lfs_config cfg;
// 初始化函数
uint8_t sd_fatfs_init(void)
{
    FRESULT res;
    
    // 挂载文件系统
    res = f_mount(&SDFatFS, SDPath, 1);
    if (res != FR_OK)
    {
        my_printf(&huart1, "SD卡挂载失败，错误码: %d\r\n", res);
        return 0;
    }
    my_printf(&huart1, "SD卡挂载成功\r\n");
    
    // 检查文件系统状态
    FATFS *fs;
    DWORD fre_clust;
    res = f_getfree(SDPath, &fre_clust, &fs);
    if (res != FR_OK)
    {
        my_printf(&huart1, "获取文件系统信息失败，错误码: %d\r\n", res);
        return 0;
    }
    my_printf(&huart1, "可用簇数: %lu\r\n", (unsigned long)fre_clust);
    
    // 创建数据目录（如果不存在）
    res = f_mkdir("DATA");
    if (res == FR_OK)
    {
        my_printf(&huart1, "创建数据目录成功\r\n");
    }
    else if (res == FR_EXIST)
    {
        my_printf(&huart1, "数据目录已存在\r\n");
    }
    else
    {
        my_printf(&huart1, "创建目录失败，错误码: %d\r\n", res);
        return 0;
    }
    
    return 1;
}


// 卸载函数
void sd_fatfs_deinit(void)
{
    f_mount(NULL, SDPath, 0);
    my_printf(&huart1, "SD卡已卸载\r\n");
}

// 读取文件内容到缓冲区
uint8_t sd_fatfs_read_file(const char* path, void* buffer, uint32_t size, uint32_t* bytes_read)
{
    FIL file;
    FRESULT res;
    UINT br;
    
    // 打开文件
    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        my_printf(&huart1, "打开文件失败，错误码: %d\r\n", res);
        return 0;
    }
    
    // 读取文件内容
    res = f_read(&file, buffer, size, &br);
    if (res != FR_OK)
    {
        my_printf(&huart1, "读取文件失败，错误码: %d\r\n", res);
        f_close(&file);
        return 0;
    }
    
    // 关闭文件
    f_close(&file);
    
    if (bytes_read != NULL)
    {
        *bytes_read = br;
    }
    
    return 1;
}

// 写入数据到文件
uint8_t sd_fatfs_write_file(const char* path, const void* buffer, uint32_t size, uint32_t* bytes_written)
{
    FIL file;
    FRESULT res;
    UINT bw;
    
    // 打开文件（如果不存在则创建）
    res = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        my_printf(&huart1, "创建/打开文件失败，错误码: %d\r\n", res);
        return 0;
    }
    
    // 写入数据
    res = f_write(&file, buffer, size, &bw);
    if (res != FR_OK)
    {
        my_printf(&huart1, "写入文件失败，错误码: %d\r\n", res);
        f_close(&file);
        return 0;
    }
    
    // 确保数据写入到存储介质
    f_sync(&file);
    
    // 关闭文件
    f_close(&file);
    
    if (bytes_written != NULL)
    {
        *bytes_written = bw;
    }
    
    return 1;
}

// 打印文件内容到串口
uint8_t sd_fatfs_print_file(const char* path)
{
    FIL file;
    FRESULT res;
    UINT br;
    char buffer[128];

    // 打开文件
    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        my_printf(&huart1, "打开文件失败，错误码: %d\r\n", res);
        return 0;
    }

    my_printf(&huart1, "文件内容:\r\n");

    // 循环读取并打印文件内容
    while (1)
    {
        res = f_read(&file, buffer, sizeof(buffer) - 1, &br);
        if (res != FR_OK || br == 0)
        {
            break;
        }

        // 确保字符串以 null 结尾
        buffer[br] = '\0';

        // 打印到串口
        my_printf(&huart1, "%s", buffer);
    }

    my_printf(&huart1, "\r\n");

    // 关闭文件
    f_close(&file);

    return 1;
}

// ==================== 统一的数据写入函数 ====================

uint8_t sd_fatfs_write_data(const char* path, DataType type, const void* data, uint32_t count)
{
    uint32_t size = 0;

    switch (type)
    {
        case DATA_TYPE_INT8:
            size = count * sizeof(int8_t);
            break;
        case DATA_TYPE_INT16:
            size = count * sizeof(int16_t);
            break;
        case DATA_TYPE_INT32:
            size = count * sizeof(int32_t);
            break;
        case DATA_TYPE_UINT8:
            size = count * sizeof(uint8_t);
            break;
        case DATA_TYPE_UINT16:
            size = count * sizeof(uint16_t);
            break;
        case DATA_TYPE_UINT32:
            size = count * sizeof(uint32_t);
            break;
        case DATA_TYPE_FLOAT:
            size = count * sizeof(float);
            break;
        case DATA_TYPE_DOUBLE:
            size = count * sizeof(double);
            break;
        case DATA_TYPE_STRING:
            size = strlen((const char*)data) + 1; // 字符串包含结束符
            break;
        case DATA_TYPE_STRUCT:
            // 对于结构体，count参数表示结构体大小
            size = count;
            break;
        default:
            my_printf(&huart1, "不支持的数据类型: %d\r\n", type);
            return 0;
    }

    return sd_fatfs_write_file(path, data, size, NULL);
}

// ==================== 统一的数据读取函数 ====================

uint8_t sd_fatfs_read_data(const char* path, DataType type, void* data, uint32_t count, uint32_t max_size)
{
    uint32_t expected_size = 0;
    uint32_t bytes_read;

    switch (type)
    {
        case DATA_TYPE_INT8:
            expected_size = count * sizeof(int8_t);
            break;
        case DATA_TYPE_INT16:
            expected_size = count * sizeof(int16_t);
            break;
        case DATA_TYPE_INT32:
            expected_size = count * sizeof(int32_t);
            break;
        case DATA_TYPE_UINT8:
            expected_size = count * sizeof(uint8_t);
            break;
        case DATA_TYPE_UINT16:
            expected_size = count * sizeof(uint16_t);
            break;
        case DATA_TYPE_UINT32:
            expected_size = count * sizeof(uint32_t);
            break;
        case DATA_TYPE_FLOAT:
            expected_size = count * sizeof(float);
            break;
        case DATA_TYPE_DOUBLE:
            expected_size = count * sizeof(double);
            break;
        case DATA_TYPE_STRING:
            // 对于字符串，使用max_size作为缓冲区大小
            if (sd_fatfs_read_file(path, data, max_size - 1, &bytes_read))
            {
                ((char*)data)[bytes_read] = '\0'; // 确保字符串结束
                return 1;
            }
            return 0;
        case DATA_TYPE_STRUCT:
            // 对于结构体，count参数表示结构体大小
            expected_size = count;
            break;
        default:
            my_printf(&huart1, "不支持的数据类型: %d\r\n", type);
            return 0;
    }

    return sd_fatfs_read_file(path, data, expected_size, &bytes_read) && (bytes_read == expected_size);
}

// ==================== 统一的数据打印函数 ====================

uint8_t sd_fatfs_print_data(const char* path, DataType type, uint32_t count)
{
    switch (type)
    {
        case DATA_TYPE_INT8:
        {
            if (count == 1)
            {
                int8_t value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (int8_t): %d\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                int8_t* array = (int8_t*)malloc(count * sizeof(int8_t));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (int8_t数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %d\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_INT16:
        {
            if (count == 1)
            {
                int16_t value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (int16_t): %d\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                int16_t* array = (int16_t*)malloc(count * sizeof(int16_t));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (int16_t数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %d\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_INT32:
        {
            if (count == 1)
            {
                int32_t value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (int32_t): %ld\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                int32_t* array = (int32_t*)malloc(count * sizeof(int32_t));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (int32_t数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %ld\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_UINT8:
        {
            if (count == 1)
            {
                uint8_t value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (uint8_t): %u\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                uint8_t* array = (uint8_t*)malloc(count * sizeof(uint8_t));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (uint8_t数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %u\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_UINT16:
        {
            if (count == 1)
            {
                uint16_t value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (uint16_t): %u\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                uint16_t* array = (uint16_t*)malloc(count * sizeof(uint16_t));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (uint16_t数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %u\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_UINT32:
        {
            if (count == 1)
            {
                uint32_t value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (uint32_t): %lu\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                uint32_t* array = (uint32_t*)malloc(count * sizeof(uint32_t));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (uint32_t数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %lu\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_FLOAT:
        {
            if (count == 1)
            {
                float value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (float): %.6f\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                float* array = (float*)malloc(count * sizeof(float));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (float数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %.6f\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_DOUBLE:
        {
            if (count == 1)
            {
                double value;
                if (sd_fatfs_read_data(path, type, &value, 1, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (double): %.6f\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                double* array = (double*)malloc(count * sizeof(double));
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (double数组[%lu]):\r\n", path, count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        my_printf(&huart1, "[%lu]: %.6f\r\n", i, array[i]);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        case DATA_TYPE_STRING:
        {
            char buffer[256];
            if (sd_fatfs_read_data(path, type, buffer, 0, sizeof(buffer)))
            {
                my_printf(&huart1, "文件 %s 内容 (string): %s\r\n", path, buffer);
                return 1;
            }
            break;
        }

        case DATA_TYPE_STRUCT:
        {
            // 根据count参数判断是SensorData还是TestStruct
            if (count == sizeof(SensorData))
            {
                // 单个SensorData结构体
                SensorData data;
                if (sd_fatfs_read_data(path, type, &data, sizeof(SensorData), 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (SensorData):\r\n", path);
                    my_printf(&huart1, "  时间戳: %lu\r\n", data.timestamp);
                    my_printf(&huart1, "  电压: %.3fV\r\n", data.voltage);
                    my_printf(&huart1, "  电流: %.3fA\r\n", data.current);
                    my_printf(&huart1, "  状态: %u\r\n", data.status);
                    return 1;
                }
            }
            else if (count % sizeof(SensorData) == 0)
            {
                // SensorData结构体数组
                uint32_t struct_count = count / sizeof(SensorData);
                SensorData* array = (SensorData*)malloc(count);
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (SensorData数组[%lu]):\r\n", path, struct_count);
                    for (uint32_t i = 0; i < struct_count; i++)
                    {
                        my_printf(&huart1, "[%lu]: 时间戳=%lu, 电压=%.3fV, 电流=%.3fA, 状态=%u\r\n",
                                  i, array[i].timestamp, array[i].voltage, array[i].current, array[i].status);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            else if (count == sizeof(TestStruct))
            {
                // 单个TestStruct结构体
                TestStruct data;
                if (sd_fatfs_read_data(path, type, &data, sizeof(TestStruct), 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (TestStruct):\r\n", path);
                    my_printf(&huart1, "  ID: %lu\r\n", data.id);
                    my_printf(&huart1, "  名称: %s\r\n", data.name);
                    my_printf(&huart1, "  数值: [%.2f, %.2f, %.2f, %.2f]\r\n",
                              data.values[0], data.values[1], data.values[2], data.values[3]);
                    my_printf(&huart1, "  标志: %u\r\n", data.flags);
                    return 1;
                }
            }
            else if (count % sizeof(TestStruct) == 0)
            {
                // TestStruct结构体数组
                uint32_t struct_count = count / sizeof(TestStruct);
                TestStruct* array = (TestStruct*)malloc(count);
                if (!array)
                {
                    my_printf(&huart1, "内存分配失败\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "文件 %s 内容 (TestStruct数组[%lu]):\r\n", path, struct_count);
                    for (uint32_t i = 0; i < struct_count; i++)
                    {
                        my_printf(&huart1, "[%lu]: ID=%lu, 名称=%s, 数值=[%.2f,%.2f,%.2f,%.2f], 标志=%u\r\n",
                                  i, array[i].id, array[i].name,
                                  array[i].values[0], array[i].values[1], array[i].values[2], array[i].values[3],
                                  array[i].flags);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            break;
        }

        default:
            my_printf(&huart1, "不支持的数据类型: %d\r\n", type);
            return 0;
    }

    my_printf(&huart1, "读取文件 %s 失败\r\n", path);
    return 0;
}
// ==================== 综合测试函数 ====================

// 测试所有数据类型的存储和读取功能
void sd_fatfs_test_all_data_types(void)
{
    my_printf(&huart1, "\r\n========== SD卡数据存储和读取测试开始 ==========\r\n");

    // 初始化SD卡
    if (!sd_fatfs_init())
    {
        my_printf(&huart1, "SD卡初始化失败，测试终止\r\n");
        return;
    }

    // 测试基本数据类型
    my_printf(&huart1, "\r\n--- 测试基本数据类型 ---\r\n");

    // 测试int8_t
    int8_t test_int8 = -123;
    if (sd_fatfs_write_data("DATA/test_int8.dat", DATA_TYPE_INT8, &test_int8, 1))
    {
        my_printf(&huart1, "写入int8_t成功: %d\r\n", test_int8);
        sd_fatfs_print_data("DATA/test_int8.dat", DATA_TYPE_INT8, 1);

        int8_t read_int8;
        if (sd_fatfs_read_data("DATA/test_int8.dat", DATA_TYPE_INT8, &read_int8, 1, 0))
        {
            my_printf(&huart1, "读取验证: %s\r\n", (read_int8 == test_int8) ? "通过" : "失败");
        }
    }

    // 测试uint16_t
    uint16_t test_uint16 = 65432;
    if (sd_fatfs_write_data("DATA/test_uint16.dat", DATA_TYPE_UINT16, &test_uint16, 1))
    {
        my_printf(&huart1, "写入uint16_t成功: %u\r\n", test_uint16);
        sd_fatfs_print_data("DATA/test_uint16.dat", DATA_TYPE_UINT16, 1);

        uint16_t read_uint16;
        if (sd_fatfs_read_data("DATA/test_uint16.dat", DATA_TYPE_UINT16, &read_uint16, 1, 0))
        {
            my_printf(&huart1, "读取验证: %s\r\n", (read_uint16 == test_uint16) ? "通过" : "失败");
        }
    }

    // 测试int32_t
    int32_t test_int32 = -1234567890;
    if (sd_fatfs_write_data("DATA/test_int32.dat", DATA_TYPE_INT32, &test_int32, 1))
    {
        my_printf(&huart1, "写入int32_t成功: %ld\r\n", test_int32);
        sd_fatfs_print_data("DATA/test_int32.dat", DATA_TYPE_INT32, 1);

        int32_t read_int32;
        if (sd_fatfs_read_data("DATA/test_int32.dat", DATA_TYPE_INT32, &read_int32, 1, 0))
        {
            my_printf(&huart1, "读取验证: %s\r\n", (read_int32 == test_int32) ? "通过" : "失败");
        }
    }

    // 测试float
    float test_float = 3.14159f;
    if (sd_fatfs_write_data("DATA/test_float.dat", DATA_TYPE_FLOAT, &test_float, 1))
    {
        my_printf(&huart1, "写入float成功: %.6f\r\n", test_float);
        sd_fatfs_print_data("DATA/test_float.dat", DATA_TYPE_FLOAT, 1);

        float read_float;
        if (sd_fatfs_read_data("DATA/test_float.dat", DATA_TYPE_FLOAT, &read_float, 1, 0))
        {
            my_printf(&huart1, "读取验证: %s\r\n", (read_float == test_float) ? "通过" : "失败");
        }
    }

    // 测试字符串
    const char* test_string = "Hello, SD Card!";
    if (sd_fatfs_write_data("DATA/test_string.dat", DATA_TYPE_STRING, test_string, 0))
    {
        my_printf(&huart1, "写入字符串成功: %s\r\n", test_string);
        sd_fatfs_print_data("DATA/test_string.dat", DATA_TYPE_STRING, 0);

        char read_string[64];
        if (sd_fatfs_read_data("DATA/test_string.dat", DATA_TYPE_STRING, read_string, 0, sizeof(read_string)))
        {
            my_printf(&huart1, "读取验证: %s\r\n", (strcmp(read_string, test_string) == 0) ? "通过" : "失败");
        }
    }

    // 测试数组
    my_printf(&huart1, "\r\n--- 测试数组类型 ---\r\n");

    // 测试int32_t数组
    int32_t test_int32_array[] = {100, -200, 300, -400, 500};
    uint32_t array_count = sizeof(test_int32_array) / sizeof(test_int32_array[0]);
    if (sd_fatfs_write_data("DATA/test_int32_array.dat", DATA_TYPE_INT32, test_int32_array, array_count))
    {
        my_printf(&huart1, "写入int32_t数组成功，元素数量: %lu\r\n", array_count);
        sd_fatfs_print_data("DATA/test_int32_array.dat", DATA_TYPE_INT32, array_count);

        int32_t read_int32_array[5];
        if (sd_fatfs_read_data("DATA/test_int32_array.dat", DATA_TYPE_INT32, read_int32_array, array_count, 0))
        {
            uint8_t match = 1;
            for (uint32_t i = 0; i < array_count; i++)
            {
                if (read_int32_array[i] != test_int32_array[i])
                {
                    match = 0;
                    break;
                }
            }
            my_printf(&huart1, "数组读取验证: %s\r\n", match ? "通过" : "失败");
        }
    }

    // 测试float数组
    float test_float_array[] = {1.1f, 2.2f, 3.3f, 4.4f};
    uint32_t float_array_count = sizeof(test_float_array) / sizeof(test_float_array[0]);
    if (sd_fatfs_write_data("DATA/test_float_array.dat", DATA_TYPE_FLOAT, test_float_array, float_array_count))
    {
        my_printf(&huart1, "写入float数组成功，元素数量: %lu\r\n", float_array_count);
        sd_fatfs_print_data("DATA/test_float_array.dat", DATA_TYPE_FLOAT, float_array_count);

        float read_float_array[4];
        if (sd_fatfs_read_data("DATA/test_float_array.dat", DATA_TYPE_FLOAT, read_float_array, float_array_count, 0))
        {
            uint8_t match = 1;
            for (uint32_t i = 0; i < float_array_count; i++)
            {
                if (read_float_array[i] != test_float_array[i])
                {
                    match = 0;
                    break;
                }
            }
            my_printf(&huart1, "float数组读取验证: %s\r\n", match ? "通过" : "失败");
        }
    }

    // 测试结构体
    my_printf(&huart1, "\r\n--- 测试结构体类型 ---\r\n");

    // 测试SensorData结构体
    SensorData test_sensor = {
        .timestamp = 1234567890,
        .voltage = 3.3f,
        .current = 0.5f,
        .status = 1
    };

    if (sd_fatfs_write_data("DATA/test_sensor.dat", DATA_TYPE_STRUCT, &test_sensor, sizeof(SensorData)))
    {
        my_printf(&huart1, "写入SensorData结构体成功\r\n");
        sd_fatfs_print_data("DATA/test_sensor.dat", DATA_TYPE_STRUCT, sizeof(SensorData));

        SensorData read_sensor;
        if (sd_fatfs_read_data("DATA/test_sensor.dat", DATA_TYPE_STRUCT, &read_sensor, sizeof(SensorData), 0))
        {
            uint8_t match = (read_sensor.timestamp == test_sensor.timestamp) &&
                           (read_sensor.voltage == test_sensor.voltage) &&
                           (read_sensor.current == test_sensor.current) &&
                           (read_sensor.status == test_sensor.status);
            my_printf(&huart1, "SensorData读取验证: %s\r\n", match ? "通过" : "失败");
        }
    }

    // 测试TestStruct结构体
    TestStruct test_struct = {
        .id = 12345,
        .name = "测试结构体",
        .values = {1.1f, 2.2f, 3.3f, 4.4f},
        .flags = 0xAB
    };

    if (sd_fatfs_write_data("DATA/test_struct.dat", DATA_TYPE_STRUCT, &test_struct, sizeof(TestStruct)))
    {
        my_printf(&huart1, "写入TestStruct结构体成功\r\n");
        sd_fatfs_print_data("DATA/test_struct.dat", DATA_TYPE_STRUCT, sizeof(TestStruct));

        TestStruct read_struct;
        if (sd_fatfs_read_data("DATA/test_struct.dat", DATA_TYPE_STRUCT, &read_struct, sizeof(TestStruct), 0))
        {
            uint8_t match = (read_struct.id == test_struct.id) &&
                           (strcmp(read_struct.name, test_struct.name) == 0) &&
                           (read_struct.flags == test_struct.flags);

            // 检查数组
            for (int i = 0; i < 4 && match; i++)
            {
                if (read_struct.values[i] != test_struct.values[i])
                {
                    match = 0;
                }
            }
            my_printf(&huart1, "TestStruct读取验证: %s\r\n", match ? "通过" : "失败");
        }
    }

    // 测试结构体数组
    my_printf(&huart1, "\r\n--- 测试结构体数组 ---\r\n");

    // 测试SensorData数组
    SensorData sensor_array[] = {
        {1000, 3.3f, 0.1f, 1},
        {2000, 3.2f, 0.2f, 1},
        {3000, 3.1f, 0.3f, 0}
    };
    uint32_t sensor_count = sizeof(sensor_array) / sizeof(sensor_array[0]);
    uint32_t sensor_array_size = sensor_count * sizeof(SensorData);

    if (sd_fatfs_write_data("DATA/test_sensor_array.dat", DATA_TYPE_STRUCT, sensor_array, sensor_array_size))
    {
        my_printf(&huart1, "写入SensorData数组成功，元素数量: %lu\r\n", sensor_count);
        sd_fatfs_print_data("DATA/test_sensor_array.dat", DATA_TYPE_STRUCT, sensor_array_size);

        SensorData read_sensor_array[3];
        if (sd_fatfs_read_data("DATA/test_sensor_array.dat", DATA_TYPE_STRUCT, read_sensor_array, sensor_array_size, 0))
        {
            uint8_t match = 1;
            for (uint32_t i = 0; i < sensor_count; i++)
            {
                if (read_sensor_array[i].timestamp != sensor_array[i].timestamp ||
                    read_sensor_array[i].voltage != sensor_array[i].voltage ||
                    read_sensor_array[i].current != sensor_array[i].current ||
                    read_sensor_array[i].status != sensor_array[i].status)
                {
                    match = 0;
                    break;
                }
            }
            my_printf(&huart1, "SensorData数组读取验证: %s\r\n", match ? "通过" : "失败");
        }
    }

    // 测试TestStruct数组
    TestStruct struct_array[] = {
        {1, "结构体1", {1.0f, 2.0f, 3.0f, 4.0f}, 0x01},
        {2, "结构体2", {5.0f, 6.0f, 7.0f, 8.0f}, 0x02},
        {3, "结构体3", {9.0f, 10.0f, 11.0f, 12.0f}, 0x03}
    };
    uint32_t struct_count = sizeof(struct_array) / sizeof(struct_array[0]);
    uint32_t struct_array_size = struct_count * sizeof(TestStruct);

    if (sd_fatfs_write_data("DATA/test_struct_array.dat", DATA_TYPE_STRUCT, struct_array, struct_array_size))
    {
        my_printf(&huart1, "写入TestStruct数组成功，元素数量: %lu\r\n", struct_count);
        sd_fatfs_print_data("DATA/test_struct_array.dat", DATA_TYPE_STRUCT, struct_array_size);

        TestStruct read_struct_array[3];
        if (sd_fatfs_read_data("DATA/test_struct_array.dat", DATA_TYPE_STRUCT, read_struct_array, struct_array_size, 0))
        {
            uint8_t match = 1;
            for (uint32_t i = 0; i < struct_count; i++)
            {
                if (read_struct_array[i].id != struct_array[i].id ||
                    strcmp(read_struct_array[i].name, struct_array[i].name) != 0 ||
                    read_struct_array[i].flags != struct_array[i].flags)
                {
                    match = 0;
                    break;
                }

                // 检查values数组
                for (int j = 0; j < 4; j++)
                {
                    if (read_struct_array[i].values[j] != struct_array[i].values[j])
                    {
                        match = 0;
                        break;
                    }
                }
                if (!match) break;
            }
            my_printf(&huart1, "TestStruct数组读取验证: %s\r\n", match ? "通过" : "失败");
        }
    }

    my_printf(&huart1, "\r\n========== SD卡数据存储和读取测试完成 ==========\r\n");
}
