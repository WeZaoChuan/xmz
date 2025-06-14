#include "dac_app.h"
#include "math.h"
#include "string.h"

// --- Copyright ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.

// --- �ⲿ HAL ������� ---
// ��Щ���ͨ���� main.c �ж��壬��Ҫ��������������ʹ��
// ��ȷ����Щ�������� CubeMX ���ɵĻ��ֶ����������һ��
extern DAC_HandleTypeDef DAC_HANDLE;
extern TIM_HandleTypeDef DAC_TIMER_HANDLE;
extern DMA_HandleTypeDef DAC_DMA_HANDLE;
#if ADC_DAC_SYNC_ENABLE
extern TIM_HandleTypeDef ADC_SYNC_TIMER_HANDLE;
#endif

// --- ˽�б��� ---
static uint16_t waveform_buffer[WAVEFORM_SAMPLES];                // �洢��ǰ�������ݵĻ�����
static dac_waveform_t current_waveform = WAVEFORM_SINE;           // ��ǰ����Ĳ�������
static uint32_t current_frequency_hz = 1000;                      // ��ǰ����Ƶ�� (Hz)
static uint16_t current_peak_amplitude_mv = DAC_VREF_MV / 2;      // ��ǰ��ֵ���� (mV)
static uint16_t dac_amplitude_raw = DAC_MAX_VALUE / 2;            // ��ǰ��ֵ���ȶ�Ӧ�� DAC ԭʼֵ
static uint8_t adc_sync_enabled = ADC_DAC_SYNC_ENABLE;            // ADCͬ��״̬
static uint8_t adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER; // ADC����Ƶ�ʱ���
static uint8_t zero_based_waveform = 1;                           // �������Ƿ�������Ĳ���

// --- ˽�к���ԭ�� ---
static void generate_waveform(void);                       // ���ݵ�ǰ�������ɲ������ݵ�������
static HAL_StatusTypeDef update_timer_frequency(void);     // ���¶�ʱ��������ƥ�䵱ǰƵ��
static HAL_StatusTypeDef start_dac_dma(void);              // ���� DAC DMA ���
static HAL_StatusTypeDef stop_dac_dma(void);               // ֹͣ DAC DMA ���
static void generate_sine(uint16_t amp_raw);               // �������Ҳ�����
static void generate_square(uint16_t amp_raw);             // ���ɷ�������
static void generate_triangle(uint16_t amp_raw);           // �������ǲ�����
static HAL_StatusTypeDef update_adc_timer_frequency(void); // ����ADC��ʱ��Ƶ����ƥ��ͬ��Ҫ��

// --- �������ɺ���ʵ�� ---
static void generate_sine(uint16_t amp_raw)
{ // �������Ҳ�
    float step = 2.0f * 3.14159265f / WAVEFORM_SAMPLES;
    if (zero_based_waveform)
    {
        // �����������Ҳ� (0 �� amp_raw*2)
        for (uint32_t i = 0; i < WAVEFORM_SAMPLES; i++)
        {
            float val = (sinf(i * step) + 1.0f) * 0.5f; // ��-1~1ӳ�䵽0~1
            int32_t dac_val = (int32_t)(val * (amp_raw * 2));
            waveform_buffer[i] = (dac_val < 0) ? 0 : ((dac_val > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)dac_val);
        }
    }
    else
    {
        // ԭʼʵ�֣������е�����Ҳ�
        for (uint32_t i = 0; i < WAVEFORM_SAMPLES; i++)
        {
            float val = sinf(i * step);
            int32_t dac_val = (int32_t)((val * amp_raw) + (DAC_MAX_VALUE / 2.0f));
            waveform_buffer[i] = (dac_val < 0) ? 0 : ((dac_val > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)dac_val);
        }
    }
}

static void generate_square(uint16_t amp_raw)
{ // ���ɷ���
    if (zero_based_waveform)
    {
        // �������ķ��� (0 �� amp_raw*2)
        uint16_t high_val = (amp_raw * 2 > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)(amp_raw * 2);
        uint16_t low_val = 0;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        for (uint32_t i = 0; i < half_samples; i++)
            waveform_buffer[i] = high_val;
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
            waveform_buffer[i] = low_val;
    }
    else
    {
        // ԭʼʵ�֣������е�ķ���
        uint16_t center = DAC_MAX_VALUE / 2;
        int32_t high_val_s = center + amp_raw;
        int32_t low_val_s = center - amp_raw;
        uint16_t high_val = (high_val_s > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)high_val_s;
        uint16_t low_val = (low_val_s < 0) ? 0 : (uint16_t)low_val_s;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        for (uint32_t i = 0; i < half_samples; i++)
            waveform_buffer[i] = high_val;
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
            waveform_buffer[i] = low_val;
    }
}

static void generate_triangle(uint16_t amp_raw)
{ // �������ǲ�
    if (zero_based_waveform)
    {
        // �����������ǲ� (0 �� amp_raw*2)
        uint16_t high_val = (amp_raw * 2 > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)(amp_raw * 2);
        uint16_t low_val = 0;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        float delta_up = (float)(high_val - low_val) / half_samples;
        float delta_down = (float)(high_val - low_val) / (WAVEFORM_SAMPLES - half_samples);

        for (uint32_t i = 0; i < half_samples; i++)
        {
            waveform_buffer[i] = low_val + (uint16_t)(i * delta_up);
        }
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
        {
            waveform_buffer[i] = high_val - (uint16_t)((i - half_samples) * delta_down);
        }
        // Ensure start/end points are correct due to potential float rounding
        if (WAVEFORM_SAMPLES > 0)
            waveform_buffer[0] = low_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples > 0)
            waveform_buffer[half_samples - 1] = high_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples < WAVEFORM_SAMPLES)
            waveform_buffer[WAVEFORM_SAMPLES - 1] = low_val;
    }
    else
    {
        // ԭʼʵ�֣������е�����ǲ�
        uint16_t center = DAC_MAX_VALUE / 2;
        int32_t peak = center + amp_raw;
        int32_t trough = center - amp_raw;
        uint16_t high_val = (peak > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)peak;
        uint16_t low_val = (trough < 0) ? 0 : (uint16_t)trough;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        float delta_up = (float)(high_val - low_val) / half_samples;
        float delta_down = (float)(high_val - low_val) / (WAVEFORM_SAMPLES - half_samples);

        for (uint32_t i = 0; i < half_samples; i++)
        {
            waveform_buffer[i] = low_val + (uint16_t)(i * delta_up);
        }
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
        {
            waveform_buffer[i] = high_val - (uint16_t)((i - half_samples) * delta_down);
        }
        // Ensure start/end points are correct due to potential float rounding
        if (WAVEFORM_SAMPLES > 0)
            waveform_buffer[0] = low_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples > 0)
            waveform_buffer[half_samples - 1] = high_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples < WAVEFORM_SAMPLES)
            waveform_buffer[WAVEFORM_SAMPLES - 1] = low_val;
    }
}

// --- ��������ʵ�� ---
static void generate_waveform(void)
{ // ���ݵ�ǰ�������ɲ���
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

static HAL_StatusTypeDef update_timer_frequency(void)
{ 
    // ��������Ч��
    if (current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0)
    {
        return HAL_ERROR;
    }

    // ����DAC����Ƶ��
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    
    // ��ȡ��ʱ��ʱ��Ƶ�ʣ�ֱ��ʹ��TIMER_INPUT_CLOCK_HZ��
    uint32_t timer_clk = TIMER_INPUT_CLOCK_HZ;
    
    // ���ڸ�Ƶ�ʣ�ʹ�ü�ֱ�ӵķ�������ARR��PSC
    uint32_t prescaler = 0; // ����ʹ���޷�Ƶ
    uint32_t arr = timer_clk / dac_update_freq - 1;
    
    // ���ARR������Χ���ټ�����ʵķ�Ƶ
    if (arr > 0xFFFF)
    {
        uint32_t div_factor = (arr / 0xFFFF) + 1;
        prescaler = div_factor - 1;
        arr = timer_clk / (dac_update_freq * (prescaler + 1)) - 1;
    }
    
    // ȷ��ARR����Ϊ1
    if (arr == 0)
    {
        arr = 1;
    }
    
    // Ӧ������
    __HAL_TIM_SET_PRESCALER(&DAC_TIMER_HANDLE, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&DAC_TIMER_HANDLE, arr);
    
    // ���ɸ����¼�������������ֵ
    HAL_TIM_GenerateEvent(&DAC_TIMER_HANDLE, TIM_EVENTSOURCE_UPDATE);
    
    // ���ADCͬ�������ã���ͬʱ����ADC��ʱ��Ƶ��
    if (adc_sync_enabled)
    {
        update_adc_timer_frequency();
    }

    return HAL_OK;
}

// --- ��������ʵ�֣�ADCͬ����� ---
static HAL_StatusTypeDef update_adc_timer_frequency(void)
{
#if ADC_DAC_SYNC_ENABLE
    if (current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0 || TIMER_INPUT_CLOCK_HZ == 0 || adc_sampling_multiplier == 0)
    {
        return HAL_ERROR; // ������Ч
    }

    // ����ADC����Ƶ�ʣ���DAC����Ƶ�ʵı���
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    uint64_t adc_sample_freq = dac_update_freq * adc_sampling_multiplier;

    // ȷ��������Ӳ������
    if (adc_sample_freq > TIMER_INPUT_CLOCK_HZ)
    {
        adc_sample_freq = TIMER_INPUT_CLOCK_HZ;
    }

    // ����ADC��ʱ��������ֵ
    uint32_t tim_period = (uint32_t)(TIMER_INPUT_CLOCK_HZ / adc_sample_freq);
    if (tim_period == 0)
        tim_period = 1;
    if (tim_period > 0xFFFF)
        tim_period = 0xFFFF;

    // ����ADC������ʱ��
    HAL_TIM_Base_Stop(&ADC_SYNC_TIMER_HANDLE);
    __HAL_TIM_SET_PRESCALER(&ADC_SYNC_TIMER_HANDLE, 0); // ����Ƶ
    __HAL_TIM_SET_AUTORELOAD(&ADC_SYNC_TIMER_HANDLE, tim_period - 1);
    HAL_TIM_GenerateEvent(&ADC_SYNC_TIMER_HANDLE, TIM_EVENTSOURCE_UPDATE);
    HAL_TIM_Base_Start(&ADC_SYNC_TIMER_HANDLE);

    return HAL_OK;
#else
    return HAL_OK; // ���δ����ͬ�����ܣ�ֱ�ӷ��سɹ�
#endif
}

static HAL_StatusTypeDef stop_dac_dma(void) // ֹͣ DAC DMA �Ͷ�ʱ��
{
    HAL_StatusTypeDef status1 = HAL_DAC_Stop_DMA(&DAC_HANDLE, DAC_CHANNEL);
    HAL_TIM_Base_Stop(&DAC_TIMER_HANDLE); // ֱ��ʹ�� Stop������鷵��ֵ��
    return status1;                       // ��Ҫ���� DAC DMA �Ƿ�ɹ�ֹͣ
}

static HAL_StatusTypeDef start_dac_dma(void)
{                                                            // ���� DAC DMA �Ͷ�ʱ��
    HAL_StatusTypeDef status_tim = update_timer_frequency(); // ȷ����ʱ��Ƶ����ȷ
    if (status_tim != HAL_OK)
        return status_tim; // ���Ƶ������ʧ��������

    HAL_StatusTypeDef status_dac = HAL_DAC_Start_DMA(&DAC_HANDLE, DAC_CHANNEL, (uint32_t *)waveform_buffer, WAVEFORM_SAMPLES, DAC_ALIGN_12B_R);
    HAL_StatusTypeDef status_tim_start = HAL_TIM_Base_Start(&DAC_TIMER_HANDLE);

    return (status_dac == HAL_OK && status_tim_start == HAL_OK) ? HAL_OK : HAL_ERROR;
}

// --- ���� API ����ʵ�� ---
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv)
{
    current_frequency_hz = (initial_freq_hz == 0) ? 1 : initial_freq_hz; // ����Ƶ��Ϊ0
    dac_app_set_amplitude(initial_peak_amplitude_mv);                    // ���ó�ʼ���� (�ڲ������ dac_amplitude_raw)
    generate_waveform();                                                 // ���ɳ�ʼ��������

    // ��ʼ��ADCͬ������
    adc_sync_enabled = ADC_DAC_SYNC_ENABLE;
    adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER;

    start_dac_dma(); // ������ʱ���� DMA ���
}

HAL_StatusTypeDef dac_app_set_waveform(dac_waveform_t type) // ���ò�������
{
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;
    current_waveform = type;
    generate_waveform(); // ʹ���µ����ͺ͵�ǰ�ķ����������ɲ���
    // Ƶ�ʲ��䣬��ʱ�����ڲ���Ҫ���¼��㣬ֻ����������
    return start_dac_dma(); // ���������ADCͬ��������start_dac_dma�е���update_adc_timer_frequency
}

HAL_StatusTypeDef dac_app_set_frequency(uint32_t freq_hz) // ����Ƶ��
{
    if (freq_hz == 0)
        return HAL_ERROR; // ������Ƶ��Ϊ 0
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;
    current_frequency_hz = freq_hz;
    // �������ͺͷ��Ȳ��䣬ֻ����¶�ʱ��������
    return start_dac_dma(); // start_dac_dma �ڲ������ update_timer_frequency��update_adc_timer_frequency
}

HAL_StatusTypeDef dac_app_set_amplitude(uint16_t peak_amplitude_mv) // ���÷�ֵ����
{
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;

    uint16_t max_amplitude_mv = DAC_VREF_MV / 2;
    current_peak_amplitude_mv = (peak_amplitude_mv > max_amplitude_mv) ? max_amplitude_mv : peak_amplitude_mv;

    // �� mV ����ת��Ϊ DAC ԭʼ����ֵ (���������ֵ DAC_MAX_VALUE / 2)
    dac_amplitude_raw = (uint16_t)(((float)current_peak_amplitude_mv / max_amplitude_mv) * (DAC_MAX_VALUE / 2.0f));
    // �ٴ�ǯλȷ���������������ֵ (��Ȼ�����ϲ���)
    if (dac_amplitude_raw > (DAC_MAX_VALUE / 2))
        dac_amplitude_raw = DAC_MAX_VALUE / 2;

    generate_waveform(); // ʹ���µķ��Ⱥ͵�ǰ�������������ɲ���
    // Ƶ�ʲ��䣬ADCͬ������start_dac_dma�д���
    return start_dac_dma();
}

// ��ȡ��ǰ��ֵ����
uint16_t dac_app_get_amplitude(void)
{
    return current_peak_amplitude_mv;
}

// --- �������� API ���� ---
HAL_StatusTypeDef dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier)
{
    if (multiplier == 0)
    {
        return HAL_ERROR; // ��������Ϊ0
    }

    // �洢�µ�����
    adc_sync_enabled = enable;
    adc_sampling_multiplier = multiplier;

    // �������ͬ��������������ADC��ʱ������
    if (enable)
    {
        return update_adc_timer_frequency();
    }

    return HAL_OK;
}

uint32_t dac_app_get_update_frequency(void)
{
    // ����DACÿ����µ����������ⲿADCͬ������
    return current_frequency_hz * WAVEFORM_SAMPLES;
}
float dac_app_get_adc_sampling_interval_us(void)
{ // ��ȡ�������ADC�������
    if (!adc_sync_enabled || current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0 || adc_sampling_multiplier == 0)

    {
        return 0.0f; // ���δ����ͬ���������Ч, ����0
    }
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    uint64_t adc_sample_freq = dac_update_freq * adc_sampling_multiplier;
    if (adc_sample_freq == 0)

    {
        return 0.0f;
    }

    return 1000000.0f / (float)adc_sample_freq;
}

// ����µĹ���API�������������ò��λ�׼ģʽ
HAL_StatusTypeDef dac_app_set_zero_based(uint8_t enable)
{
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;

    zero_based_waveform = enable ? 1 : 0;

    generate_waveform(); // ʹ���µ�ģʽ�������ɲ���
    return start_dac_dma();
}

// ��ȡ��ǰ���λ�׼ģʽ
uint8_t dac_app_get_zero_based(void)
{
    return zero_based_waveform;
}
