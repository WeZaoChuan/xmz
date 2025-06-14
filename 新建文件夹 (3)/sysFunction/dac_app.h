#ifndef __DAC_APP_H
#define __DAC_APP_H

#include "mydefine.h"

// --- Copyright ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.

// --- ���� (���������Ӳ���� CubeMX �����޸�) ---
#define DAC_RESOLUTION_BITS 12                         // DAC �ֱ��� (λ��)
#define DAC_MAX_VALUE ((1 << DAC_RESOLUTION_BITS) - 1) // DAC �������ֵ (���� 12λ -> 4095)
#define DAC_VREF_MV 3300                               // DAC �ο���ѹ (����)
#define WAVEFORM_SAMPLES 256                           // ÿ���������ڵĲ������� (Ӱ���ڴ�����Ƶ��)
#define TIMER_INPUT_CLOCK_HZ 90000000                  // ���뵽 DAC ��ʱ����ʱ��Ƶ�� (Hz), �����ʵ�������޸�!
#define DAC_TIMER_HANDLE htim6                         // ���ڴ��� DAC �� TIM ��� (���� CubeMX)
#define DAC_HANDLE hdac                                // DAC ��� (���� CubeMX)
#define DAC_CHANNEL DAC_CHANNEL_1                      // ʹ�õ� DAC ͨ�� (���� CubeMX)
// ע��: ����� DMA �������Ҫ���� CubeMX ��Ϊ DAC ͨ�����õ� DMA ��ȷ��
#define DAC_DMA_HANDLE hdma_dac_ch1 // DAC DMA ͨ����� (���� CubeMX)

// --- ADC����ͬ������ ---
#define ADC_DAC_SYNC_ENABLE 1       // �Ƿ�����ADC��DACͬ�� (0:����, 1:����)
#define ADC_SYNC_TIMER_HANDLE htim3 // ���ڴ���ADC�Ķ�ʱ�����
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
 * @brief ���������������
 * @param type: �������� (dac_waveform_t ö��)
 * @retval HAL_StatusTypeDef: ����״̬
 */
HAL_StatusTypeDef dac_app_set_waveform(dac_waveform_t type);

/**
 * @brief �����������Ƶ��
 * @param freq_hz: �µ�Ƶ�� (Hz)
 * @retval HAL_StatusTypeDef: ����״̬
 */
HAL_StatusTypeDef dac_app_set_frequency(uint32_t freq_hz);

/**
 * @brief ����������η�ֵ����
 * @param peak_amplitude_mv: �µķ�ֵ���� (����, 0 �� Vref/2)
 * @retval HAL_StatusTypeDef: ����״̬
 */
HAL_StatusTypeDef dac_app_set_amplitude(uint16_t peak_amplitude_mv);

/**
 * @brief ����ADC����ͬ������
 * @param enable: �Ƿ�����ͬ�� (0:����, 1:����)
 * @param multiplier: ADC����Ƶ�������DAC����Ƶ�ʵı���
 * @retval HAL_StatusTypeDef: ����״̬
 */
HAL_StatusTypeDef dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier);

/**
 * @brief ��ȡ��ǰDAC����Ƶ�ʣ������ⲿADCͬ������
 * @retval uint32_t: DACÿ����µ��� (Hz * WAVEFORM_SAMPLES)
 */
uint32_t dac_app_get_update_frequency(void);

/**
 * @brief ��ȡ��ǰ���η�ֵ����
 * @retval uint16_t: ��ǰ��ֵ����(����)
 */
uint16_t dac_app_get_amplitude(void);

float dac_app_get_adc_sampling_interval_us(void);

/**
 * @brief ���ò��λ�׼ģʽ
 * @param enable: �Ƿ�������(0:�����е�, 1:�������)
 * @retval HAL_StatusTypeDef: ����״̬
 */
HAL_StatusTypeDef dac_app_set_zero_based(uint8_t enable);

/**
 * @brief ��ȡ��ǰ���λ�׼ģʽ
 * @retval uint8_t: ��ǰģʽ(0:�����е�, 1:�������)
 */
uint8_t dac_app_get_zero_based(void);
#endif // __DAC_APP_H
