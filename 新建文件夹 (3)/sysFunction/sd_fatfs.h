#ifndef __SD_FATFS_H_
#define __SD_FATFS_H_


#include "ff.h"
#include "diskio.h"
#include "mydefine.h"

// ���崫�������ݽṹ��
typedef struct {
    uint32_t timestamp;  // ʱ���
    float voltage;       // ��ѹֵ
    float current;       // ����ֵ
    uint8_t status;      // ״̬��־
}SensorData;

// ��������õĸ��ӽṹ��
typedef struct {
    uint32_t id;
    char name[32];
    float values[4];
    uint8_t flags;
} TestStruct;

// ��������ö��
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

// ������������
uint8_t sd_fatfs_init(void);
void sd_fatfs_deinit(void);

// �����ļ�������������
uint8_t sd_fatfs_read_file(const char* path, void* buffer, uint32_t size, uint32_t* bytes_read);
uint8_t sd_fatfs_write_file(const char* path, const void* buffer, uint32_t size, uint32_t* bytes_written);
uint8_t sd_fatfs_print_file(const char* path);

// ͳһ������д�뺯��
uint8_t sd_fatfs_write_data(const char* path, DataType type, const void* data, uint32_t count);

// ͳһ�����ݶ�ȡ����
uint8_t sd_fatfs_read_data(const char* path, DataType type, void* data, uint32_t count, uint32_t max_size);

// ͳһ�����ݴ�ӡ����
uint8_t sd_fatfs_print_data(const char* path, DataType type, uint32_t count);

// ���Ժ�������
void sd_fatfs_test_all_data_types(void);

#endif /* __SD_FATFS_H */

