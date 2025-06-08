#ifndef __DAC_APP_H
#define __DAC_APP_H

#include "stdint.h"
#include "gd32f4xx.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- ���ò��� ---
#define DAC_RESOLUTION_BITS 12                         // DAC �ֱ��� (λ��)
#define DAC_MAX_VALUE ((1 << DAC_RESOLUTION_BITS) - 1) // DAC �������ֵ (���� 12λ -> 4095)
#define DAC_VREF_MV 3300                               // DAC �ο���ѹ (����)
#define WAVEFORM_SAMPLES 256                           // ÿ���������ڵĲ�������
#define TIMER_INPUT_CLOCK_HZ 240000000                 // ���뵽 DAC ��ʱ����ʱ��Ƶ�� (Hz)

// --- ADC����ͬ������ ---
#define ADC_DAC_SYNC_ENABLE 1       // �Ƿ�����ADC��DACͬ�� (0:����, 1:����)
#define ADC_SAMPLING_MULTIPLIER 1   // ADC����Ƶ�������DAC����Ƶ�ʵı���

// --- ��������ö�� ---
typedef enum
{
    WAVEFORM_SINE,    // ���Ҳ�
    WAVEFORM_SQUARE,  // ����
    WAVEFORM_TRIANGLE // ���ǲ�
} dac_waveform_t;

// --- ����ԭ�� ---
/**
 * @brief ��ʼ�� DAC Ӧ�ÿ�
 * @param initial_freq_hz: ��ʼ����Ƶ�� (Hz)
 * @param initial_peak_amplitude_mv: ��ʼ���η�ֵ���� (����, ������ĵ�ѹ Vref/2)
 * @retval None
 */
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv);

/**
 * @brief ���ò�������
 * @param type: �������� (WAVEFORM_SINE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_set_waveform(dac_waveform_t type);

/**
 * @brief ���ò���Ƶ��
 * @param freq_hz: ����Ƶ�� (Hz)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_set_frequency(uint32_t freq_hz);

/**
 * @brief ���ò��η�ֵ����
 * @param peak_amplitude_mv: ��ֵ���� (����, ������ĵ�ѹ Vref/2)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_set_amplitude(uint16_t peak_amplitude_mv);

/**
 * @brief ��ȡ��ǰ���η�ֵ����
 * @retval uint16_t: ��ǰ��ֵ����(����)
 */
uint16_t dac_app_get_amplitude(void);

/**
 * @brief ��ȡ��ǰ����Ƶ��
 * @retval uint32_t: ��ǰƵ��(Hz)
 */
uint32_t dac_app_get_frequency(void);

/**
 * @brief ��ȡ��ǰ��������
 * @retval dac_waveform_t: ��ǰ��������
 */
dac_waveform_t dac_app_get_waveform(void);

/**
 * @brief ��ȡ��ǰDAC����Ƶ�ʣ������ⲿADCͬ������
 * @retval uint32_t: DACÿ����µ��� (Hz * WAVEFORM_SAMPLES)
 */
uint32_t dac_app_get_update_frequency(void);

/**
 * @brief ����ADCͬ������
 * @param enable: �Ƿ�����ͬ�� (0:����, 1:����)
 * @param multiplier: ADC����Ƶ�������DAC����Ƶ�ʵı���
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier);

/**
 * @brief ��ȡ�������ADC�������
 * @retval float: ADC�������(΢��)
 */
float dac_app_get_adc_sampling_interval_us(void);

/**
 * @brief ���ò��λ�׼ģʽ
 * @param enable: �Ƿ�������(0:�����е�, 1:�������)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_set_zero_based(uint8_t enable);

/**
 * @brief ��ȡ��ǰ���λ�׼ģʽ
 * @retval uint8_t: ��ǰģʽ(0:�����е�, 1:�������)
 */
uint8_t dac_app_get_zero_based(void);

/**
 * @brief ����DAC���
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_start(void);

/**
 * @brief ֹͣDAC���
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int dac_app_stop(void);

// ԭ�к������ּ���
void dac_task(void);

#ifdef __cplusplus
}
#endif

#endif /* __DAC_APP_H */
