#ifndef __SD_APP_H_
#define __SD_APP_H_

#include "stdint.h"
typedef enum {
    DATA_TYPE_INT8,      // 8λ�з�������
    DATA_TYPE_UINT8,     // 8λ�޷�������
    DATA_TYPE_INT16,     // 16λ�з�������
    DATA_TYPE_UINT16,    // 16λ�޷�������
    DATA_TYPE_INT32,     // 32λ�з�������
    DATA_TYPE_UINT32,    // 32λ�޷�������
    DATA_TYPE_FLOAT,     // �����ȸ�����
    DATA_TYPE_DOUBLE,    // ˫���ȸ�����
    DATA_TYPE_STRING,    // �ַ�������
    DATA_TYPE_BINARY     // ���������ݣ�Ĭ�ϸ�ʽ��
} data_type_t;
void print_adc_file_info(void);
void sd_fatfs_init(void);
void sd_fatfs_test(void);
uint8_t sd_fatfs_save_data(const char *filename, void *data, uint32_t data_size, uint8_t max_retry);
uint32_t sd_fatfs_read_data(const char *filename, void *data, uint32_t max_size, uint8_t max_retry);
void print_data(const void *data, uint32_t data_size, data_type_t type);
#endif /* __SD_APP_H_ */
