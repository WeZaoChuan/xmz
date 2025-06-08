#include "dac_app.h"
#include "mcu_cmic_gd32f470vet6.h"
#include "systick.h"
#include <string.h>

// --- Copyright ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.
// Adapted for GD32F4xx standard library

// --- ˽�б��� ---
static uint16_t waveform_buffer[WAVEFORM_SAMPLES];                // �洢��ǰ�������ݵĻ�����
static dac_waveform_t current_waveform = WAVEFORM_SINE;           // ��ǰ����Ĳ�������
static uint32_t current_frequency_hz = 1000;                      // ��ǰ����Ƶ�� (Hz)
static uint16_t current_peak_amplitude_mv = DAC_VREF_MV / 2;      // ��ǰ��ֵ���� (mV)
static uint16_t dac_amplitude_raw = DAC_MAX_VALUE / 2;            // ��ǰ��ֵ���ȶ�Ӧ�� DAC ԭʼֵ
static uint8_t adc_sync_enabled = ADC_DAC_SYNC_ENABLE;            // ADCͬ��״̬
static uint8_t adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER; // ADC����Ƶ�ʱ���
static uint8_t zero_based_waveform = 1;                           // �Ƿ�������Ĳ���
static uint8_t dac_running = 0;                                   // DAC����״̬

// --- ˽�к���ԭ�� ---
static void generate_waveform(void);                       // ���ݵ�ǰ�������ɲ������ݵ�������
static int update_timer_frequency(void);                   // ���¶�ʱ��������ƥ�䵱ǰƵ��
static int start_dac_dma(void);                           // ���� DAC DMA ���
static int stop_dac_dma(void);                            // ֹͣ DAC DMA ���
static void generate_sine(uint16_t amp_raw);               // �������Ҳ�����
static void generate_square(uint16_t amp_raw);             // ���ɷ�������
static void generate_triangle(uint16_t amp_raw);           // �������ǲ�����
static int update_adc_timer_frequency(void);              // ����ADC��ʱ��Ƶ����ƥ��ͬ��Ҫ��

// --- ˽�к���ʵ�� ---

// �������Ҳ�����
static void generate_sine(uint16_t amp_raw)
{
    uint16_t center_val = DAC_MAX_VALUE / 2;
    
    for (int i = 0; i < WAVEFORM_SAMPLES; i++)
    {
        float angle = 2.0f * M_PI * i / WAVEFORM_SAMPLES;
        float sine_val = sinf(angle);
        
        if (zero_based_waveform)
        {
            // ������㣺0 �� amp_raw*2
            waveform_buffer[i] = (uint16_t)((sine_val + 1.0f) * amp_raw);
        }
        else
        {
            // �����е㣺center_val �� amp_raw
            waveform_buffer[i] = (uint16_t)(center_val + sine_val * amp_raw);
        }
        
        // ȷ��������DAC��Χ
        if (waveform_buffer[i] > DAC_MAX_VALUE)
            waveform_buffer[i] = DAC_MAX_VALUE;
    }
}

// ���ɷ�������
static void generate_square(uint16_t amp_raw)
{
    uint16_t center_val = DAC_MAX_VALUE / 2;
    uint16_t high_val, low_val;
    
    if (zero_based_waveform)
    {
        high_val = amp_raw * 2;
        low_val = 0;
    }
    else
    {
        high_val = center_val + amp_raw;
        low_val = center_val - amp_raw;
    }
    
    // ȷ��������DAC��Χ
    if (high_val > DAC_MAX_VALUE) high_val = DAC_MAX_VALUE;
    if (low_val > DAC_MAX_VALUE) low_val = 0;
    
    int half_samples = WAVEFORM_SAMPLES / 2;
    
    for (int i = 0; i < half_samples; i++)
    {
        waveform_buffer[i] = high_val;
    }
    for (int i = half_samples; i < WAVEFORM_SAMPLES; i++)
    {
        waveform_buffer[i] = low_val;
    }
}

// �������ǲ�����
static void generate_triangle(uint16_t amp_raw)
{
    uint16_t center_val = DAC_MAX_VALUE / 2;
    uint16_t high_val, low_val;
    
    if (zero_based_waveform)
    {
        high_val = amp_raw * 2;
        low_val = 0;
    }
    else
    {
        high_val = center_val + amp_raw;
        low_val = center_val - amp_raw;
    }
    
    // ȷ��������DAC��Χ
    if (high_val > DAC_MAX_VALUE) high_val = DAC_MAX_VALUE;
    if (low_val > DAC_MAX_VALUE) low_val = 0;
    
    int half_samples = WAVEFORM_SAMPLES / 2;
    
    // ������
    for (int i = 0; i < half_samples; i++)
    {
        waveform_buffer[i] = low_val + (uint16_t)((float)(high_val - low_val) * i / half_samples);
    }
    
    // �½���
    for (int i = half_samples; i < WAVEFORM_SAMPLES; i++)
    {
        waveform_buffer[i] = high_val - (uint16_t)((float)(high_val - low_val) * (i - half_samples) / half_samples);
    }
    
    // ȷ����ʼ�ͽ�������ȷ
    if (WAVEFORM_SAMPLES > 0)
        waveform_buffer[0] = low_val;
    if (WAVEFORM_SAMPLES > 1)
        waveform_buffer[WAVEFORM_SAMPLES - 1] = low_val;
}

// ���ݵ�ǰ�������ɲ���
static void generate_waveform(void)
{
    switch (current_waveform)
    {
    case WAVEFORM_SINE:
        generate_sine(dac_amplitude_raw);
        break;
    case WAVEFORM_SQUARE:
        generate_square(dac_amplitude_raw);
        break;
    case WAVEFORM_TRIANGLE:
        generate_triangle(dac_amplitude_raw);
        break;
    default:
        generate_sine(dac_amplitude_raw);
        break; // Ĭ��Ϊ���Ҳ�
    }
}

// ���¶�ʱ��Ƶ��
static int update_timer_frequency(void)
{
    if (current_frequency_hz == 0) return -1;
    
    uint32_t dac_update_freq = current_frequency_hz * WAVEFORM_SAMPLES;
    uint32_t timer_period = TIMER_INPUT_CLOCK_HZ / dac_update_freq;
    
    if (timer_period == 0) timer_period = 1;
    if (timer_period > 65535) timer_period = 65535;
    
    // ֹͣ��ʱ��
    timer_disable(TIMER5);
    
    // �������ö�ʱ��
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = 239;  // Ԥ��Ƶ����ʹʱ��Ϊ1MHz
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (timer_period / 240) - 1;  // ��������
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    
    timer_init(TIMER5, &timer_initpara);
    timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);
    
    return 0;
}

// ����DAC DMA���
static int start_dac_dma(void)
{
    if (update_timer_frequency() != 0) return -1;
    
    // ����DMA��������ַ�ʹ�С
    dma_channel_disable(DMA0, DMA_CH5);
    
    // �ȴ�DMAͨ���������
    while (dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_FTF));
    
    // ��������DMA
    DMA_CHPADDR(DMA0, DMA_CH5) = DAC0_R12DH_ADDRESS;
    DMA_CHM0ADDR(DMA0, DMA_CH5) = (uint32_t)waveform_buffer;
    DMA_CHCNT(DMA0, DMA_CH5) = WAVEFORM_SAMPLES;
    
    // ����DMAͨ��
    dma_channel_enable(DMA0, DMA_CH5);
    
    // ������ʱ��
    timer_enable(TIMER5);
    
    dac_running = 1;
    
    // �������ADCͬ��������ADC��ʱ��
    if (adc_sync_enabled)
    {
        update_adc_timer_frequency();
    }
    
    return 0;
}

// ֹͣDAC DMA���
static int stop_dac_dma(void)
{
    // ֹͣ��ʱ��
    timer_disable(TIMER5);
    
    // ֹͣDMA
    dma_channel_disable(DMA0, DMA_CH5);
    
    dac_running = 0;
    
    return 0;
}

// ����ADC��ʱ��Ƶ����ƥ��ͬ��Ҫ��
static int update_adc_timer_frequency(void)
{
    // ������Ҫ���ݾ����ADC��ʱ��������ʵ��
    // ��ʱ���سɹ�
    return 0;
}

// --- ���� API ����ʵ�� ---
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv)
{
    current_frequency_hz = (initial_freq_hz == 0) ? 1 : initial_freq_hz;
    dac_app_set_amplitude(initial_peak_amplitude_mv);
    generate_waveform();
    
    // ��ʼ��ADCͬ������
    adc_sync_enabled = ADC_DAC_SYNC_ENABLE;
    adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER;
    
    // ��ʼ��Ӳ��
    bsp_dac_init();
    
    start_dac_dma();
}

int dac_app_set_waveform(dac_waveform_t type)
{
    if (stop_dac_dma() != 0) return -1;
    
    current_waveform = type;
    generate_waveform();
    
    return start_dac_dma();
}

int dac_app_set_frequency(uint32_t freq_hz)
{
    if (freq_hz == 0) return -1;
    if (stop_dac_dma() != 0) return -1;
    
    current_frequency_hz = freq_hz;
    
    return start_dac_dma();
}

int dac_app_set_amplitude(uint16_t peak_amplitude_mv)
{
    if (stop_dac_dma() != 0) return -1;
    
    uint16_t max_amplitude_mv = DAC_VREF_MV / 2;
    current_peak_amplitude_mv = (peak_amplitude_mv > max_amplitude_mv) ? max_amplitude_mv : peak_amplitude_mv;
    
    // �� mV ����ת��Ϊ DAC ԭʼ����ֵ
    dac_amplitude_raw = (uint16_t)(((float)current_peak_amplitude_mv / max_amplitude_mv) * (DAC_MAX_VALUE / 2.0f));
    if (dac_amplitude_raw > (DAC_MAX_VALUE / 2))
        dac_amplitude_raw = DAC_MAX_VALUE / 2;
    
    generate_waveform();
    
    return start_dac_dma();
}

uint16_t dac_app_get_amplitude(void)
{
    return current_peak_amplitude_mv;
}

uint32_t dac_app_get_frequency(void)
{
    return current_frequency_hz;
}

dac_waveform_t dac_app_get_waveform(void)
{
    return current_waveform;
}

uint32_t dac_app_get_update_frequency(void)
{
    return current_frequency_hz * WAVEFORM_SAMPLES;
}

int dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier)
{
    if (multiplier == 0) return -1;
    
    adc_sync_enabled = enable;
    adc_sampling_multiplier = multiplier;
    
    if (enable && dac_running)
    {
        return update_adc_timer_frequency();
    }
    
    return 0;
}

float dac_app_get_adc_sampling_interval_us(void)
{
    if (!adc_sync_enabled || current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0 || adc_sampling_multiplier == 0)
    {
        return 0.0f;
    }
    
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    uint64_t adc_sample_freq = dac_update_freq * adc_sampling_multiplier;
    
    if (adc_sample_freq == 0) return 0.0f;
    
    return 1000000.0f / (float)adc_sample_freq;
}

int dac_app_set_zero_based(uint8_t enable)
{
    if (stop_dac_dma() != 0) return -1;
    
    zero_based_waveform = enable ? 1 : 0;
    generate_waveform();
    
    return start_dac_dma();
}

uint8_t dac_app_get_zero_based(void)
{
    return zero_based_waveform;
}

int dac_app_start(void)
{
    return start_dac_dma();
}

int dac_app_stop(void)
{
    return stop_dac_dma();
}

// ԭ�к������ּ���
void dac_task(void)
{
    // �������������������������״̬��ص�
}
