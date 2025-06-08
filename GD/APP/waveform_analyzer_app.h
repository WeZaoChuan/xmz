#ifndef __WAVEFORM_ANALYZER_APP_H
#define __WAVEFORM_ANALYZER_APP_H

#include "stdint.h"
#include "gd32f4xx.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- ���ò��� ---
#define FFT_SIZE 256                    // FFT����
#define SAMPLE_RATE_HZ 10000           // Ĭ�ϲ�����
#define FREQUENCY_RESOLUTION (SAMPLE_RATE_HZ / FFT_SIZE)  // Ƶ�ʷֱ���

// --- �������Ͷ��� ---
typedef enum
{
    ADC_WAVEFORM_DC = 0,       // ֱ���ź�
    ADC_WAVEFORM_SINE = 1,     // ���Ҳ�
    ADC_WAVEFORM_SQUARE = 2,   // ����
    ADC_WAVEFORM_TRIANGLE = 3, // ���ǲ�
    ADC_WAVEFORM_UNKNOWN = 255 // δ֪����
} ADC_WaveformType;

// --- г��������Ϣ�ṹ�� ---
typedef struct
{
    float frequency;    // г��Ƶ��
    float amplitude;    // г������
    float phase;        // г����λ
    float relative_amp; // ����ڻ����ķ��ȱ�
} HarmonicComponent;

// --- ��չ�Ĳ�����Ϣ�ṹ�� ---
typedef struct
{
    ADC_WaveformType waveform_type;   // ��������ö��
    float frequency;                  // ����Ƶ�ʣ���λHz
    float vpp;                        // ���ֵ����λV
    float mean;                       // ��ֵ����λV
    float rms;                        // ��Чֵ����λV
    float phase;                      // ������λ����λ����
    HarmonicComponent third_harmonic; // ����г����Ϣ
    HarmonicComponent fifth_harmonic; // ���г����Ϣ
    float thd;                        // ��г��ʧ��
    float snr;                        // �����
} WaveformInfo;

// --- FFT��ؽṹ�� ---
typedef struct
{
    float real;
    float imag;
} complex_t;

// --- ȫ�ֱ������� ---
extern float fft_input_buffer[FFT_SIZE];
extern complex_t fft_output_buffer[FFT_SIZE];
extern float magnitude_buffer[FFT_SIZE / 2];
extern float phase_buffer[FFT_SIZE / 2];

// --- ��ʼ������ ---
/**
 * @brief ��ʼ�����η�����
 * @param sample_rate: ������(Hz)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int waveform_analyzer_init(uint32_t sample_rate);

// --- Ƶ��ӳ�亯�� ---
/**
 * @brief ������Ƶ��ӳ�䵽FFTƵ��
 * @param input_frequency: ����Ƶ��
 * @retval float: FFTƵ��
 */
float Map_Input_To_FFT_Frequency(float input_frequency);

/**
 * @brief ��FFTƵ��ӳ�䵽����Ƶ��
 * @param fft_frequency: FFTƵ��
 * @retval float: ����Ƶ��
 */
float Map_FFT_To_Input_Frequency(float fft_frequency);

// --- �������β����������� ---
/**
 * @brief ��ȡ���η��ֵ����ֵ����Чֵ
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @param mean: �����ֵ
 * @param rms: �����Чֵ
 * @retval float: ���ֵ
 */
float Get_Waveform_Vpp(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *mean, float *rms);

/**
 * @brief ��ȡ����Ƶ��
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @retval float: Ƶ��(Hz)
 */
float Get_Waveform_Frequency(const uint16_t *adc_val_buffer, uint16_t buffer_size);

/**
 * @brief ʶ��������
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @retval ADC_WaveformType: ��������
 */
ADC_WaveformType Get_Waveform_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size);

// --- FFT�������� ---
/**
 * @brief ִ��FFT�任
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int Perform_FFT(const uint16_t *adc_val_buffer, uint16_t buffer_size);

/**
 * @brief ����Ƶ�ʺͲ�������
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @param signal_frequency: ����ź�Ƶ��
 * @retval ADC_WaveformType: ��������
 */
ADC_WaveformType Analyze_Frequency_And_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *signal_frequency);

// --- ��λ�������� ---
/**
 * @brief ��ȡ������λ
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @param frequency: �ź�Ƶ��
 * @retval float: ��λ(����)
 */
float Get_Waveform_Phase(const uint16_t *adc_val_buffer, uint16_t buffer_size, float frequency);

/**
 * @brief ͨ����������ȡ������λ
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @param frequency: �ź�Ƶ��
 * @retval float: ��λ(����)
 */
float Get_Waveform_Phase_ZeroCrossing(const uint16_t *adc_val_buffer, uint16_t buffer_size, float frequency);

/**
 * @brief ����������λ֮��Ĳ�ֵ
 * @param phase1: ��λ1
 * @param phase2: ��λ2
 * @retval float: ��λ��
 */
float Calculate_Phase_Difference(float phase1, float phase2);

/**
 * @brief ��ȡ�����ź�֮�����λ��
 * @param adc_val_buffer1: ��һ���źŻ�����
 * @param adc_val_buffer2: �ڶ����źŻ�����
 * @param buffer_size: ��������С
 * @param frequency: �ź�Ƶ��
 * @retval float: ��λ��
 */
float Get_Phase_Difference(const uint16_t *adc_val_buffer1, const uint16_t *adc_val_buffer2, 
                          uint16_t buffer_size, float frequency);

// --- г���������� ---
/**
 * @brief ����г���ɷ�
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @param waveform_info: ���������Ϣ
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int Analyze_Harmonics(const uint16_t *adc_val_buffer, uint16_t buffer_size, WaveformInfo *waveform_info);

/**
 * @brief ��ȡFFT��������λ
 * @param fft_buffer: FFT������
 * @param component_idx: ��������
 * @retval float: ��λ
 */
float Get_Component_Phase(const complex_t *fft_buffer, int component_idx);

/**
 * @brief ������г��ʧ��(THD)
 * @param fundamental_amp: ��������
 * @param harmonic_amps: г����������
 * @param harmonic_count: г������
 * @retval float: THDֵ
 */
float Calculate_THD(float fundamental_amp, const float *harmonic_amps, int harmonic_count);

/**
 * @brief ���������(SNR)
 * @param signal_power: �źŹ���
 * @param noise_power: ��������
 * @retval float: SNRֵ(dB)
 */
float Calculate_SNR(float signal_power, float noise_power);

// --- �������� ---
/**
 * @brief ��ȡ���������ַ���
 * @param waveform: ��������
 * @retval const char*: ���������ַ���
 */
const char *GetWaveformTypeString(ADC_WaveformType waveform);

/**
 * @brief ��ADCֵת��Ϊ��ѹ
 * @param adc_value: ADCֵ
 * @retval float: ��ѹֵ(V)
 */
float ADC_To_Voltage(uint16_t adc_value);

/**
 * @brief ����ѹת��ΪADCֵ
 * @param voltage: ��ѹֵ(V)
 * @retval uint16_t: ADCֵ
 */
uint16_t Voltage_To_ADC(float voltage);

// --- �ۺϽӿ� ---
/**
 * @brief ��ȡ�����Ĳ�����Ϣ
 * @param adc_val_buffer: ADC���ݻ�����
 * @param buffer_size: ��������С
 * @retval WaveformInfo: ������Ϣ�ṹ��
 */
WaveformInfo Get_Waveform_Info(const uint16_t *adc_val_buffer, uint16_t buffer_size);

/**
 * @brief ���ò�����
 * @param sample_rate: ������(Hz)
 * @retval 0: �ɹ�, ��0: ʧ��
 */
int Set_Sample_Rate(uint32_t sample_rate);

/**
 * @brief ��ȡ��ǰ������
 * @retval uint32_t: ������(Hz)
 */
uint32_t Get_Sample_Rate(void);

#ifdef __cplusplus
}
#endif

#endif // __WAVEFORM_ANALYZER_APP_H
