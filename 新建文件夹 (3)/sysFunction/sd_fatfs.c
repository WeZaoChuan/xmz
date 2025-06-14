#include "sd_fatfs.h"
#include "usart_app.h"
#include "string.h"
#include "fatfs.h"
#include "usart.h"
#include "stdlib.h"


//lfs_t lfs;
//struct lfs_config cfg;
// ��ʼ������
uint8_t sd_fatfs_init(void)
{
    FRESULT res;
    
    // �����ļ�ϵͳ
    res = f_mount(&SDFatFS, SDPath, 1);
    if (res != FR_OK)
    {
        my_printf(&huart1, "SD������ʧ�ܣ�������: %d\r\n", res);
        return 0;
    }
    my_printf(&huart1, "SD�����سɹ�\r\n");
    
    // ����ļ�ϵͳ״̬
    FATFS *fs;
    DWORD fre_clust;
    res = f_getfree(SDPath, &fre_clust, &fs);
    if (res != FR_OK)
    {
        my_printf(&huart1, "��ȡ�ļ�ϵͳ��Ϣʧ�ܣ�������: %d\r\n", res);
        return 0;
    }
    my_printf(&huart1, "���ô���: %lu\r\n", (unsigned long)fre_clust);
    
    // ��������Ŀ¼����������ڣ�
    res = f_mkdir("DATA");
    if (res == FR_OK)
    {
        my_printf(&huart1, "��������Ŀ¼�ɹ�\r\n");
    }
    else if (res == FR_EXIST)
    {
        my_printf(&huart1, "����Ŀ¼�Ѵ���\r\n");
    }
    else
    {
        my_printf(&huart1, "����Ŀ¼ʧ�ܣ�������: %d\r\n", res);
        return 0;
    }
    
    return 1;
}


// ж�غ���
void sd_fatfs_deinit(void)
{
    f_mount(NULL, SDPath, 0);
    my_printf(&huart1, "SD����ж��\r\n");
}

// ��ȡ�ļ����ݵ�������
uint8_t sd_fatfs_read_file(const char* path, void* buffer, uint32_t size, uint32_t* bytes_read)
{
    FIL file;
    FRESULT res;
    UINT br;
    
    // ���ļ�
    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        my_printf(&huart1, "���ļ�ʧ�ܣ�������: %d\r\n", res);
        return 0;
    }
    
    // ��ȡ�ļ�����
    res = f_read(&file, buffer, size, &br);
    if (res != FR_OK)
    {
        my_printf(&huart1, "��ȡ�ļ�ʧ�ܣ�������: %d\r\n", res);
        f_close(&file);
        return 0;
    }
    
    // �ر��ļ�
    f_close(&file);
    
    if (bytes_read != NULL)
    {
        *bytes_read = br;
    }
    
    return 1;
}

// д�����ݵ��ļ�
uint8_t sd_fatfs_write_file(const char* path, const void* buffer, uint32_t size, uint32_t* bytes_written)
{
    FIL file;
    FRESULT res;
    UINT bw;
    
    // ���ļ�������������򴴽���
    res = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        my_printf(&huart1, "����/���ļ�ʧ�ܣ�������: %d\r\n", res);
        return 0;
    }
    
    // д������
    res = f_write(&file, buffer, size, &bw);
    if (res != FR_OK)
    {
        my_printf(&huart1, "д���ļ�ʧ�ܣ�������: %d\r\n", res);
        f_close(&file);
        return 0;
    }
    
    // ȷ������д�뵽�洢����
    f_sync(&file);
    
    // �ر��ļ�
    f_close(&file);
    
    if (bytes_written != NULL)
    {
        *bytes_written = bw;
    }
    
    return 1;
}

// ��ӡ�ļ����ݵ�����
uint8_t sd_fatfs_print_file(const char* path)
{
    FIL file;
    FRESULT res;
    UINT br;
    char buffer[128];

    // ���ļ�
    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        my_printf(&huart1, "���ļ�ʧ�ܣ�������: %d\r\n", res);
        return 0;
    }

    my_printf(&huart1, "�ļ�����:\r\n");

    // ѭ����ȡ����ӡ�ļ�����
    while (1)
    {
        res = f_read(&file, buffer, sizeof(buffer) - 1, &br);
        if (res != FR_OK || br == 0)
        {
            break;
        }

        // ȷ���ַ����� null ��β
        buffer[br] = '\0';

        // ��ӡ������
        my_printf(&huart1, "%s", buffer);
    }

    my_printf(&huart1, "\r\n");

    // �ر��ļ�
    f_close(&file);

    return 1;
}

// ==================== ͳһ������д�뺯�� ====================

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
            size = strlen((const char*)data) + 1; // �ַ�������������
            break;
        case DATA_TYPE_STRUCT:
            // ���ڽṹ�壬count������ʾ�ṹ���С
            size = count;
            break;
        default:
            my_printf(&huart1, "��֧�ֵ���������: %d\r\n", type);
            return 0;
    }

    return sd_fatfs_write_file(path, data, size, NULL);
}

// ==================== ͳһ�����ݶ�ȡ���� ====================

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
            // �����ַ�����ʹ��max_size��Ϊ��������С
            if (sd_fatfs_read_file(path, data, max_size - 1, &bytes_read))
            {
                ((char*)data)[bytes_read] = '\0'; // ȷ���ַ�������
                return 1;
            }
            return 0;
        case DATA_TYPE_STRUCT:
            // ���ڽṹ�壬count������ʾ�ṹ���С
            expected_size = count;
            break;
        default:
            my_printf(&huart1, "��֧�ֵ���������: %d\r\n", type);
            return 0;
    }

    return sd_fatfs_read_file(path, data, expected_size, &bytes_read) && (bytes_read == expected_size);
}

// ==================== ͳһ�����ݴ�ӡ���� ====================

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
                    my_printf(&huart1, "�ļ� %s ���� (int8_t): %d\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                int8_t* array = (int8_t*)malloc(count * sizeof(int8_t));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (int8_t����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (int16_t): %d\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                int16_t* array = (int16_t*)malloc(count * sizeof(int16_t));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (int16_t����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (int32_t): %ld\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                int32_t* array = (int32_t*)malloc(count * sizeof(int32_t));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (int32_t����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (uint8_t): %u\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                uint8_t* array = (uint8_t*)malloc(count * sizeof(uint8_t));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (uint8_t����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (uint16_t): %u\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                uint16_t* array = (uint16_t*)malloc(count * sizeof(uint16_t));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (uint16_t����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (uint32_t): %lu\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                uint32_t* array = (uint32_t*)malloc(count * sizeof(uint32_t));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (uint32_t����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (float): %.6f\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                float* array = (float*)malloc(count * sizeof(float));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (float����[%lu]):\r\n", path, count);
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
                    my_printf(&huart1, "�ļ� %s ���� (double): %.6f\r\n", path, value);
                    return 1;
                }
            }
            else
            {
                double* array = (double*)malloc(count * sizeof(double));
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (double����[%lu]):\r\n", path, count);
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
                my_printf(&huart1, "�ļ� %s ���� (string): %s\r\n", path, buffer);
                return 1;
            }
            break;
        }

        case DATA_TYPE_STRUCT:
        {
            // ����count�����ж���SensorData����TestStruct
            if (count == sizeof(SensorData))
            {
                // ����SensorData�ṹ��
                SensorData data;
                if (sd_fatfs_read_data(path, type, &data, sizeof(SensorData), 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (SensorData):\r\n", path);
                    my_printf(&huart1, "  ʱ���: %lu\r\n", data.timestamp);
                    my_printf(&huart1, "  ��ѹ: %.3fV\r\n", data.voltage);
                    my_printf(&huart1, "  ����: %.3fA\r\n", data.current);
                    my_printf(&huart1, "  ״̬: %u\r\n", data.status);
                    return 1;
                }
            }
            else if (count % sizeof(SensorData) == 0)
            {
                // SensorData�ṹ������
                uint32_t struct_count = count / sizeof(SensorData);
                SensorData* array = (SensorData*)malloc(count);
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (SensorData����[%lu]):\r\n", path, struct_count);
                    for (uint32_t i = 0; i < struct_count; i++)
                    {
                        my_printf(&huart1, "[%lu]: ʱ���=%lu, ��ѹ=%.3fV, ����=%.3fA, ״̬=%u\r\n",
                                  i, array[i].timestamp, array[i].voltage, array[i].current, array[i].status);
                    }
                    free(array);
                    return 1;
                }
                free(array);
            }
            else if (count == sizeof(TestStruct))
            {
                // ����TestStruct�ṹ��
                TestStruct data;
                if (sd_fatfs_read_data(path, type, &data, sizeof(TestStruct), 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (TestStruct):\r\n", path);
                    my_printf(&huart1, "  ID: %lu\r\n", data.id);
                    my_printf(&huart1, "  ����: %s\r\n", data.name);
                    my_printf(&huart1, "  ��ֵ: [%.2f, %.2f, %.2f, %.2f]\r\n",
                              data.values[0], data.values[1], data.values[2], data.values[3]);
                    my_printf(&huart1, "  ��־: %u\r\n", data.flags);
                    return 1;
                }
            }
            else if (count % sizeof(TestStruct) == 0)
            {
                // TestStruct�ṹ������
                uint32_t struct_count = count / sizeof(TestStruct);
                TestStruct* array = (TestStruct*)malloc(count);
                if (!array)
                {
                    my_printf(&huart1, "�ڴ����ʧ��\r\n");
                    return 0;
                }

                if (sd_fatfs_read_data(path, type, array, count, 0))
                {
                    my_printf(&huart1, "�ļ� %s ���� (TestStruct����[%lu]):\r\n", path, struct_count);
                    for (uint32_t i = 0; i < struct_count; i++)
                    {
                        my_printf(&huart1, "[%lu]: ID=%lu, ����=%s, ��ֵ=[%.2f,%.2f,%.2f,%.2f], ��־=%u\r\n",
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
            my_printf(&huart1, "��֧�ֵ���������: %d\r\n", type);
            return 0;
    }

    my_printf(&huart1, "��ȡ�ļ� %s ʧ��\r\n", path);
    return 0;
}
// ==================== �ۺϲ��Ժ��� ====================

// ���������������͵Ĵ洢�Ͷ�ȡ����
void sd_fatfs_test_all_data_types(void)
{
    my_printf(&huart1, "\r\n========== SD�����ݴ洢�Ͷ�ȡ���Կ�ʼ ==========\r\n");

    // ��ʼ��SD��
    if (!sd_fatfs_init())
    {
        my_printf(&huart1, "SD����ʼ��ʧ�ܣ�������ֹ\r\n");
        return;
    }

    // ���Ի�����������
    my_printf(&huart1, "\r\n--- ���Ի����������� ---\r\n");

    // ����int8_t
    int8_t test_int8 = -123;
    if (sd_fatfs_write_data("DATA/test_int8.dat", DATA_TYPE_INT8, &test_int8, 1))
    {
        my_printf(&huart1, "д��int8_t�ɹ�: %d\r\n", test_int8);
        sd_fatfs_print_data("DATA/test_int8.dat", DATA_TYPE_INT8, 1);

        int8_t read_int8;
        if (sd_fatfs_read_data("DATA/test_int8.dat", DATA_TYPE_INT8, &read_int8, 1, 0))
        {
            my_printf(&huart1, "��ȡ��֤: %s\r\n", (read_int8 == test_int8) ? "ͨ��" : "ʧ��");
        }
    }

    // ����uint16_t
    uint16_t test_uint16 = 65432;
    if (sd_fatfs_write_data("DATA/test_uint16.dat", DATA_TYPE_UINT16, &test_uint16, 1))
    {
        my_printf(&huart1, "д��uint16_t�ɹ�: %u\r\n", test_uint16);
        sd_fatfs_print_data("DATA/test_uint16.dat", DATA_TYPE_UINT16, 1);

        uint16_t read_uint16;
        if (sd_fatfs_read_data("DATA/test_uint16.dat", DATA_TYPE_UINT16, &read_uint16, 1, 0))
        {
            my_printf(&huart1, "��ȡ��֤: %s\r\n", (read_uint16 == test_uint16) ? "ͨ��" : "ʧ��");
        }
    }

    // ����int32_t
    int32_t test_int32 = -1234567890;
    if (sd_fatfs_write_data("DATA/test_int32.dat", DATA_TYPE_INT32, &test_int32, 1))
    {
        my_printf(&huart1, "д��int32_t�ɹ�: %ld\r\n", test_int32);
        sd_fatfs_print_data("DATA/test_int32.dat", DATA_TYPE_INT32, 1);

        int32_t read_int32;
        if (sd_fatfs_read_data("DATA/test_int32.dat", DATA_TYPE_INT32, &read_int32, 1, 0))
        {
            my_printf(&huart1, "��ȡ��֤: %s\r\n", (read_int32 == test_int32) ? "ͨ��" : "ʧ��");
        }
    }

    // ����float
    float test_float = 3.14159f;
    if (sd_fatfs_write_data("DATA/test_float.dat", DATA_TYPE_FLOAT, &test_float, 1))
    {
        my_printf(&huart1, "д��float�ɹ�: %.6f\r\n", test_float);
        sd_fatfs_print_data("DATA/test_float.dat", DATA_TYPE_FLOAT, 1);

        float read_float;
        if (sd_fatfs_read_data("DATA/test_float.dat", DATA_TYPE_FLOAT, &read_float, 1, 0))
        {
            my_printf(&huart1, "��ȡ��֤: %s\r\n", (read_float == test_float) ? "ͨ��" : "ʧ��");
        }
    }

    // �����ַ���
    const char* test_string = "Hello, SD Card!";
    if (sd_fatfs_write_data("DATA/test_string.dat", DATA_TYPE_STRING, test_string, 0))
    {
        my_printf(&huart1, "д���ַ����ɹ�: %s\r\n", test_string);
        sd_fatfs_print_data("DATA/test_string.dat", DATA_TYPE_STRING, 0);

        char read_string[64];
        if (sd_fatfs_read_data("DATA/test_string.dat", DATA_TYPE_STRING, read_string, 0, sizeof(read_string)))
        {
            my_printf(&huart1, "��ȡ��֤: %s\r\n", (strcmp(read_string, test_string) == 0) ? "ͨ��" : "ʧ��");
        }
    }

    // ��������
    my_printf(&huart1, "\r\n--- ������������ ---\r\n");

    // ����int32_t����
    int32_t test_int32_array[] = {100, -200, 300, -400, 500};
    uint32_t array_count = sizeof(test_int32_array) / sizeof(test_int32_array[0]);
    if (sd_fatfs_write_data("DATA/test_int32_array.dat", DATA_TYPE_INT32, test_int32_array, array_count))
    {
        my_printf(&huart1, "д��int32_t����ɹ���Ԫ������: %lu\r\n", array_count);
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
            my_printf(&huart1, "�����ȡ��֤: %s\r\n", match ? "ͨ��" : "ʧ��");
        }
    }

    // ����float����
    float test_float_array[] = {1.1f, 2.2f, 3.3f, 4.4f};
    uint32_t float_array_count = sizeof(test_float_array) / sizeof(test_float_array[0]);
    if (sd_fatfs_write_data("DATA/test_float_array.dat", DATA_TYPE_FLOAT, test_float_array, float_array_count))
    {
        my_printf(&huart1, "д��float����ɹ���Ԫ������: %lu\r\n", float_array_count);
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
            my_printf(&huart1, "float�����ȡ��֤: %s\r\n", match ? "ͨ��" : "ʧ��");
        }
    }

    // ���Խṹ��
    my_printf(&huart1, "\r\n--- ���Խṹ������ ---\r\n");

    // ����SensorData�ṹ��
    SensorData test_sensor = {
        .timestamp = 1234567890,
        .voltage = 3.3f,
        .current = 0.5f,
        .status = 1
    };

    if (sd_fatfs_write_data("DATA/test_sensor.dat", DATA_TYPE_STRUCT, &test_sensor, sizeof(SensorData)))
    {
        my_printf(&huart1, "д��SensorData�ṹ��ɹ�\r\n");
        sd_fatfs_print_data("DATA/test_sensor.dat", DATA_TYPE_STRUCT, sizeof(SensorData));

        SensorData read_sensor;
        if (sd_fatfs_read_data("DATA/test_sensor.dat", DATA_TYPE_STRUCT, &read_sensor, sizeof(SensorData), 0))
        {
            uint8_t match = (read_sensor.timestamp == test_sensor.timestamp) &&
                           (read_sensor.voltage == test_sensor.voltage) &&
                           (read_sensor.current == test_sensor.current) &&
                           (read_sensor.status == test_sensor.status);
            my_printf(&huart1, "SensorData��ȡ��֤: %s\r\n", match ? "ͨ��" : "ʧ��");
        }
    }

    // ����TestStruct�ṹ��
    TestStruct test_struct = {
        .id = 12345,
        .name = "���Խṹ��",
        .values = {1.1f, 2.2f, 3.3f, 4.4f},
        .flags = 0xAB
    };

    if (sd_fatfs_write_data("DATA/test_struct.dat", DATA_TYPE_STRUCT, &test_struct, sizeof(TestStruct)))
    {
        my_printf(&huart1, "д��TestStruct�ṹ��ɹ�\r\n");
        sd_fatfs_print_data("DATA/test_struct.dat", DATA_TYPE_STRUCT, sizeof(TestStruct));

        TestStruct read_struct;
        if (sd_fatfs_read_data("DATA/test_struct.dat", DATA_TYPE_STRUCT, &read_struct, sizeof(TestStruct), 0))
        {
            uint8_t match = (read_struct.id == test_struct.id) &&
                           (strcmp(read_struct.name, test_struct.name) == 0) &&
                           (read_struct.flags == test_struct.flags);

            // �������
            for (int i = 0; i < 4 && match; i++)
            {
                if (read_struct.values[i] != test_struct.values[i])
                {
                    match = 0;
                }
            }
            my_printf(&huart1, "TestStruct��ȡ��֤: %s\r\n", match ? "ͨ��" : "ʧ��");
        }
    }

    // ���Խṹ������
    my_printf(&huart1, "\r\n--- ���Խṹ������ ---\r\n");

    // ����SensorData����
    SensorData sensor_array[] = {
        {1000, 3.3f, 0.1f, 1},
        {2000, 3.2f, 0.2f, 1},
        {3000, 3.1f, 0.3f, 0}
    };
    uint32_t sensor_count = sizeof(sensor_array) / sizeof(sensor_array[0]);
    uint32_t sensor_array_size = sensor_count * sizeof(SensorData);

    if (sd_fatfs_write_data("DATA/test_sensor_array.dat", DATA_TYPE_STRUCT, sensor_array, sensor_array_size))
    {
        my_printf(&huart1, "д��SensorData����ɹ���Ԫ������: %lu\r\n", sensor_count);
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
            my_printf(&huart1, "SensorData�����ȡ��֤: %s\r\n", match ? "ͨ��" : "ʧ��");
        }
    }

    // ����TestStruct����
    TestStruct struct_array[] = {
        {1, "�ṹ��1", {1.0f, 2.0f, 3.0f, 4.0f}, 0x01},
        {2, "�ṹ��2", {5.0f, 6.0f, 7.0f, 8.0f}, 0x02},
        {3, "�ṹ��3", {9.0f, 10.0f, 11.0f, 12.0f}, 0x03}
    };
    uint32_t struct_count = sizeof(struct_array) / sizeof(struct_array[0]);
    uint32_t struct_array_size = struct_count * sizeof(TestStruct);

    if (sd_fatfs_write_data("DATA/test_struct_array.dat", DATA_TYPE_STRUCT, struct_array, struct_array_size))
    {
        my_printf(&huart1, "д��TestStruct����ɹ���Ԫ������: %lu\r\n", struct_count);
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

                // ���values����
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
            my_printf(&huart1, "TestStruct�����ȡ��֤: %s\r\n", match ? "ͨ��" : "ʧ��");
        }
    }

    my_printf(&huart1, "\r\n========== SD�����ݴ洢�Ͷ�ȡ������� ==========\r\n");
}
