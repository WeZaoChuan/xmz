#ifndef __ADC_APP_H_
#define __ADC_APP_H_

#include "stdint.h"
#include "gd32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- ���ò��� ---
#define ADC_BUFFER_SIZE 1024                    // ADC��������С
#define ADC_RESOLUTION_BITS 12                  // ADC�ֱ���
#define ADC_MAX_VALUE ((1 << ADC_RESOLUTION_BITS) - 1)  // ADC���ֵ
#define ADC_VREF_MV 3300                        // ADC�ο���ѹ(����)
#define ADC_TIMER_CLOCK_HZ 240000000            // ADC��ʱ��ʱ��Ƶ��

// --- ADC����ģʽ ---
typedef enum
{
    ADC_MODE_SINGLE,        // ���β���
    ADC_MODE_CONTINUOUS,    // ��������
    ADC_MODE_TIMER_TRIGGER, // ��ʱ����������
    ADC_MODE_DMA_CIRCULAR   // DMAѭ������
} adc_mode_t;

// --- ADC״̬ ---
typedef enum
{
    ADC_STATE_IDLE,         // ����״̬
    ADC_STATE_SAMPLING,     // ������
    ADC_STATE_COMPLETE,     // �������
    ADC_STATE_ERROR         // ����״̬
} adc_state_t;

// --- ADC���ýṹ�� ---
typedef struct
{
    adc_mode_t mode;                // ����ģʽ
    uint32_t sample_rate_hz;        // ����Ƶ��(Hz)
    uint16_t buffer_size;           // ��������С
    uint8_t channel;                // ADCͨ��
    uint8_t trigger_source;         // ����Դ
} adc_config_t;

// --- ȫ�ֱ������� ---
extern uint8_t adc_recording_flag;     // ADC¼�Ʊ�־
extern char adc_filename[32];          // ADC�ļ���
extern uint16_t adc_buffer[ADC_BUFFER_SIZE];  // ADC���ݻ�����
extern volatile adc_state_t adc_current_state;  // ADC��ǰ״̬

// --- �߼�ADC���� ---
/**
 * @brief ��ʼ���߼�ADC����
 * @param config: ADC���ò���
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int adc_app_init(const adc_config_t *config);

/**
 * @brief ����ADC����
 * @param samples: �������� (0��ʾ��������)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int adc_app_start_sampling(uint16_t samples);

/**
 * @brief ֹͣADC����
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int adc_app_stop_sampling(void);

/**
 * @brief ����ADC����Ƶ��
 * @param sample_rate_hz: ����Ƶ��(Hz)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int adc_app_set_sample_rate(uint32_t sample_rate_hz);

/**
 * @brief ��ȡADC����Ƶ��
 * @retval uint32_t: ��ǰ����Ƶ��(Hz)
 */
uint32_t adc_app_get_sample_rate(void);

/**
 * @brief ����ADC��ʱ������
 * @param timer_freq_hz: ��ʱ��Ƶ��(Hz)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int adc_app_config_timer_trigger(uint32_t timer_freq_hz);

/**
 * @brief ��ȡADC״̬
 * @retval adc_state_t: ��ǰADC״̬
 */
adc_state_t adc_app_get_state(void);

/**
 * @brief ��ȡADC���ݻ�����
 * @param buffer: ���������ָ��
 * @param size: ��������С
 * @retval uint16_t: ʵ�ʻ�ȡ�����ݵ���
 */
uint16_t adc_app_get_data(uint16_t *buffer, uint16_t size);

/**
 * @brief ��ADCԭʼֵת��Ϊ��ѹ(����)
 * @param raw_value: ADCԭʼֵ
 * @retval uint16_t: ��ѹֵ(����)
 */
uint16_t adc_app_raw_to_mv(uint16_t raw_value);

/**
 * @brief ����ѹ(����)ת��ΪADCԭʼֵ
 * @param voltage_mv: ��ѹֵ(����)
 * @retval uint16_t: ADCԭʼֵ
 */
uint16_t adc_app_mv_to_raw(uint16_t voltage_mv);

/**
 * @brief ��ȡADC����ͳ����Ϣ
 * @param buffer: ���ݻ�����
 * @param size: ���ݴ�С
 * @param min: �����Сֵ
 * @param max: ������ֵ
 * @param mean: ���ƽ��ֵ
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int adc_app_get_statistics(const uint16_t *buffer, uint16_t size,
                          uint16_t *min, uint16_t *max, float *mean);

/**
 * @brief DMA������ɻص�����
 */
void adc_app_dma_complete_callback(void);

/**
 * @brief DMA�������ɻص�����
 */
void adc_app_dma_half_complete_callback(void);

// --- ԭ�к������ּ��� ---
void adc_task(void);
void adc_start_recording(void);
void adc_stop_recording(void);
void adc_recording_init(void);
void adc_check_filesystem(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_APP_H_ */
