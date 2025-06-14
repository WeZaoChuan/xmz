#include "adc_app.h"
#include "tim.h"
#include "dac_app.h"
#include "sd_fatfs.h"

// 1 ��ѯ
// 2 DMA����ת��
// 3 DMA TIM ��ͨ���ɼ�
#define ADC_MODE (3)

// --- ���� ADC ��ش��� ---

#if ADC_MODE == 1

__IO uint32_t adc_val; // ���ڴ洢������ƽ�� ADC ֵ
__IO float voltage;    // ���ڴ洢�����ĵ�ѹֵ

// ����Ҫ��ȡ ADC �ĵط����ã�����һ����������
void adc_task(void)
{
    // 1. ���� ADC ת��
    HAL_ADC_Start(&hadc1); // hadc1 ����� ADC ���

    // 2. �ȴ�ת����� (����ʽ)
    //    ���� 1000 ��ʾ��ʱʱ�� (����)
    if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK)
    {
        // 3. ת���ɹ�����ȡ���ֽ�� (0-4095 for 12-bit)
        adc_val = HAL_ADC_GetValue(&hadc1);

        // 4. (��ѡ) ������ֵת��Ϊʵ�ʵ�ѹֵ
        //    ���� Vref = 3.3V, �ֱ��� 12 λ (4096)
        voltage = (float)adc_val * 3.3f / 4096.0f;

        // (������Լ������ voltage �� adc_val �Ĵ����߼�)
        my_printf(&huart1, "ADC Value: %lu, Voltage: %.2fV\n", adc_val, voltage);
    }
    else
    {
        // ת����ʱ�������
        // my_printf(&huart1, "ADC Poll Timeout!\n");
    }

    // 5. ����Ҫ����� ADC ����Ϊ����ת��ģʽ��ͨ������Ҫ�ֶ�ֹͣ��
    //    ���������ת��ģʽ��������Ҫ HAL_ADC_Stop(&hadc1);
    // HAL_ADC_Stop(&hadc1); // ������� CubeMX ���þ����Ƿ���Ҫ
}

#elif ADC_MODE == 2

// --- ȫ�ֱ��� ---
#define ADC_DMA_BUFFER_SIZE 32 // DMA��������С�����Ը�����Ҫ����
uint32_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE]; // DMA Ŀ�껺����
__IO uint32_t adc_val;                        // ���ڴ洢������ƽ�� ADC ֵ
__IO float voltage;                           // ���ڴ洢�����ĵ�ѹֵ

// --- ��ʼ�� (ͨ���� main �����������ʼ�������е���һ��) ---
void adc_dma_init(void)
{
    // ���� ADC ��ʹ�� DMA ����
    // hadc1: ADC ���
    // (uint32_t*)adc_dma_buffer: DMA Ŀ�껺������ַ (HAL��ͨ����Ҫuint32_t*)
    // ADC_DMA_BUFFER_SIZE: ���δ���������� (��������С)

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_DMA_BUFFER_SIZE);
}

// --- �������� (����ѭ����ʱ���ص��ж��ڵ���) ---
void adc_task(void)
{
    uint32_t adc_sum = 0;

    // 1. ���� DMA �����������в���ֵ���ܺ�
    //    ע�⣺����ֱ�Ӷ�ȡ�����������ܰ�����ͬʱ�̵Ĳ���ֵ
    for (uint16_t i = 0; i < ADC_DMA_BUFFER_SIZE; i++)
    {
        adc_sum += adc_dma_buffer[i];
    }

    // 2. ����ƽ�� ADC ֵ
    adc_val = adc_sum / ADC_DMA_BUFFER_SIZE;

    // 3. (��ѡ) ��ƽ������ֵת��Ϊʵ�ʵ�ѹֵ
    voltage = ((float)adc_val * 3.3f) / 4096.0f; // ����12λ�ֱ���, 3.3V�ο���ѹ

    // 4. ʹ�ü������ƽ��ֵ (adc_val �� voltage)
    my_printf(&huart1, "Average ADC: %lu, Voltage: %.2fV\n", adc_val, voltage);
}

#elif ADC_MODE == 3

#define BUFFER_SIZE 2048

extern DMA_HandleTypeDef hdma_adc1;

uint32_t adc_value_buffer[BUFFER_SIZE / 2];
uint32_t dac_val_buffer[BUFFER_SIZE / 2];
uint32_t res_val_buffer[BUFFER_SIZE / 2];
__IO uint32_t adc_val_buffer[BUFFER_SIZE];
__IO float voltage;
__IO uint8_t AdcConvEnd = 0;
uint8_t wave_analysis_flag = 0; // ���η�����־λ
uint8_t wave_query_type = 0;    // ���β�ѯ���ͣ�0=ȫ��, 1=����, 2=Ƶ��, 3=���ֵ

// ADC���ݼ�¼��ر���
volatile uint8_t adc_record_flag = 0;     // ���ݼ�¼��־
volatile uint8_t adc_print_flag = 0;      // ���ݴ�ӡ��־
AdcDataRecord adc_data_buffer[MAX_ADC_RECORDS];  // ���ݻ�����
volatile uint16_t adc_record_count = 0;   // ��ǰ��¼����
static uint8_t adc_record_counter = 0;    // ��¼���������������10ms�������

void adc_tim_dma_init(void)
{
    HAL_TIM_Base_Start(&htim3);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT); // ���ð봫���ж�
}

// ADC ת����ɻص�
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    UNUSED(hadc);
    if (hadc == &hadc1) // ȷ���� hadc1 ���
    {
        HAL_ADC_Stop_DMA(hadc); // ֹͣ DMA���ȴ�����
        AdcConvEnd = 1;         // ���ñ�־λ
    }
}

// ��usart_app.c������Ϊextern
WaveformInfo wave_data;

void adc_task(void)
{
    // һ������ת�� 3(����) + 12.5(ת��) = 15.5 ADCʱ������
    // ���� ADC ʱ�� 14MHz (���� HSI/PLL), һ��ת��ʱ��: 15.5 / 14MHz ~= 1.1 us
    // BUFFER_SIZE ��ת����ʱ��: 1000 * 1.1 us = 1.1 ms (����ֵ)
    // ��ʱ������Ƶ����Ҫƥ��������ʻ����
    // ��ǰ����������Ϊ1msִ��һ�Σ����ݼ�¼��Ϊ10ms���

    // ÿ10ms��¼һ��ADC���ݣ�������DMA��ɱ�־
    if (adc_record_flag && adc_record_count < MAX_ADC_RECORDS)
    {
			  uint32_t   adc_sum=0;
        adc_record_counter++;
        if (adc_record_counter >= 10) // ÿ10��1ms���ڼ�¼һ�Σ���10ms���
        {
           adc_record_counter = 0; // ���ü�����
					 for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
          {
           adc_value_buffer[i] = adc_val_buffer[i * 2 ]; // �� ADC ���ݴ�����Ϊ dac_val_buffer ����
          }
				  for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
          {
           adc_sum += res_val_buffer[i];
          }
          uint32_t current_sample = adc_sum / (BUFFER_SIZE / 2); // ȡ��һ���������������ݵ�ʵʱ�仯
          float current_voltage = (float)current_sample * 3.3f / 4096.0f;

          // ��¼����
          adc_data_buffer[adc_record_count].timestamp = HAL_GetTick();
          adc_data_buffer[adc_record_count].adc_value = current_sample;
          adc_data_buffer[adc_record_count].voltage = current_voltage;
          adc_record_count++;
        }

            // ������������ˣ��Զ�ֹͣ��¼������
            if (adc_record_count >= MAX_ADC_RECORDS)
            {
                adc_record_flag = 0;
                my_printf(&huart1, "ADC���ݼ�¼����������¼%d�����ݣ����ڱ��浽SD��...\r\n", adc_record_count);
                adc_save_data_to_sd();
            }
        }

    if (AdcConvEnd) // ���ת����ɱ�־
    {
        // --- ����ɼ��������� ---
        // ʾ�������������������ݸ��Ƶ���һ�������� 
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            dac_val_buffer[i] = adc_val_buffer[i * 2 + 1]; // �� ADC ���ݴ�����Ϊ dac_val_buffer ������
            res_val_buffer[i] = adc_val_buffer[i * 2];
        }
        uint32_t res_sum = 0;
        // �� res_val_buffer �е�����ת��Ϊ��ѹֵ
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            res_sum += res_val_buffer[i];
        }
				
        uint32_t res_avg = res_sum / (BUFFER_SIZE / 2);
        voltage = (float)res_avg * 3.3f / 4096.0f;
        // �� voltage (0-3.3V) ӳ�䵽 DAC ��ֵ���� (0-1650mV������ VREF=3.3V)
        dac_app_set_amplitude((uint16_t)(voltage * 500.0f));

        // ע�⣺ADC���ݼ�¼�߼����Ƶ�adc_task��ͷ��ÿ10msִ��һ��

        if (uart_send_flag == 1)
        {
            // ʾ����ͨ�����ڴ�ӡ����������
            for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
            {
                // ע��: ԭ�����ʽ���ַ��� "{dac}%d\r\n" ���������ض����������Ա���
                my_printf(&huart1, "{dac}%d\r\n", (int)dac_val_buffer[i]);
            }
        }

        // ��������˲��η�����־������в��η���
        if (wave_analysis_flag)
        {
            wave_data = Get_Waveform_Info(dac_val_buffer);

            // ���ݲ�ѯ���ʹ�ӡ��Ӧ����Ϣ
            switch (wave_query_type)
            {
            case 4: // ��ӡȫ����Ϣ
                my_printf(&huart1, "����Ƶ��: %d FFTƵ�ʣ�%d ���ֵ��%.2f �������ͣ�%s\r\n",
                          dac_app_get_update_frequency() / WAVEFORM_SAMPLES,
                          (uint32_t)wave_data.frequency,
                          wave_data.vpp,
                          GetWaveformTypeString(wave_data.waveform_type));
                break;

            case 1: // ����ӡ��������
                my_printf(&huart1, "��ǰ��������: %s\r\n", GetWaveformTypeString(wave_data.waveform_type));
                break;

            case 2: // ����ӡƵ��
                my_printf(&huart1, "��ǰƵ��: %dHz\r\n", (uint32_t)wave_data.frequency);
                break;

            case 3: // ����ӡ���ֵ
                my_printf(&huart1, "��ǰ���ֵ: %.2fmV\r\n", wave_data.vpp);
                break;
            }

            wave_analysis_flag = 0; // ������ɺ������־
            wave_query_type = 0;    // ���ò�ѯ����
        }

        // --- ������� ---

        // ��մ������� (��ѡ��ȡ���ں����߼�)
        // memset(dac_val_buffer, 0, sizeof(uint32_t) * (BUFFER_SIZE / 2));

        // ��� ADC DMA �������ͱ�־λ��׼����һ�βɼ�
        // memset(adc_val_buffer, 0, sizeof(uint32_t) * BUFFER_SIZE); // ���ԭʼ���� (�����Ҫ)
        AdcConvEnd = 0;

        // �������� ADC DMA ������һ�βɼ�
        // ע�⣺�����ʱ������������ ADC �ģ����ܲ���Ҫ�ֶ�ֹͣ/���� DMA
        // ��Ҫ���� TIM3 �� ADC �ľ������þ����Ƿ���Ҫ��������
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
        __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT); // �ٴν��ð봫���ж�
    }
}


// ADC���ݼ�¼���ƺ���
void adc_start_recording(void)
{
    adc_record_count = 0;    // ���ü�¼������
    adc_record_counter = 0;  // ���ü��������
    adc_record_flag = 1;     // ��ʼ��¼
    my_printf(&huart1, "��ʼADC���ݼ�¼��10ms�����...\r\n");
}

void adc_stop_recording(void)
{
    adc_record_flag = 0;   // ֹͣ��¼
    my_printf(&huart1, "ֹͣADC���ݼ�¼������¼%d������\r\n", adc_record_count);
}

void adc_save_data_to_sd(void)
{
    if (adc_record_count == 0)
    {
        my_printf(&huart1, "û��������Ҫ����\r\n");
        return;
    }

    // �����ܵ����ݴ�С
    uint32_t total_size = adc_record_count * sizeof(AdcDataRecord);

    // ���浽SD��
    if (sd_fatfs_write_data("DATA/adc_records.dat", DATA_TYPE_STRUCT, adc_data_buffer, total_size))
    {
        my_printf(&huart1, "ADC�����ѱ��浽SD������%d����¼\r\n", adc_record_count);
    }
    else
    {
        my_printf(&huart1, "ADC���ݱ���ʧ��\r\n");
    }
}

void adc_print_data_from_sd(void)
{
    // ���ȳ��Զ�ȡ����
    AdcDataRecord temp_buffer[MAX_ADC_RECORDS];
    uint32_t bytes_read;

    // ��ȡ�ļ���ȡʵ�ʴ�С
    if (!sd_fatfs_read_file("DATA/adc_records.dat", temp_buffer, sizeof(temp_buffer), &bytes_read))
    {
        my_printf(&huart1, "��ȡADC�����ļ�ʧ��\r\n");
        return;
    }

    // �����¼����
    uint32_t record_count = bytes_read / sizeof(AdcDataRecord);

    if (record_count == 0)
    {
        my_printf(&huart1, "û���ҵ�ADC���ݼ�¼\r\n");
        return;
    }

    my_printf(&huart1, "=== ADC���ݼ�¼ (��%lu��) ===\r\n", record_count);

    // ������ӡ����
    for (uint32_t i = 0; i < record_count; i++)
    {
        my_printf(&huart1, "ʱ�����%lu��ADCֵ��%lu����ѹֵ��%f\r\n",
                  temp_buffer[i].timestamp, temp_buffer[i].adc_value,temp_buffer[i].voltage);
    }

    my_printf(&huart1, "=== ���ݴ�ӡ��� ===\r\n");
}

#endif
