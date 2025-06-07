#include "mcu_cmic_gd32f470vet6.h"

extern uint16_t adc_value[1];
extern uint16_t convertarr[CONVERT_NUM];

// ȫ�ֱ�������
uint8_t adc_recording_flag = 0; // ADC���ݲɼ�״̬��־��0-ֹͣ��1-�ɼ���
char adc_filename[32] = {0}; // ADC�����ļ���
static FIL adc_file; // ADC�����ļ����
static uint32_t last_adc_time = 0; // �ϴ�ADC�ɼ�ʱ��

// ���η�����ر���
#define ADC_BUFFER_SIZE 1024
static uint16_t adc_buffer[ADC_BUFFER_SIZE];
static uint32_t adc_buffer_index = 0;
static uint32_t last_zero_crossing = 0;
static uint32_t zero_crossing_count = 0;
static uint32_t last_analysis_time = 0;

// ��������
float calculate_frequency_from_adc(void);
float calculate_peak_to_peak_from_adc(void);
void analyze_waveform(void);

void adc_task(void)
{
    convertarr[0] = adc_value[0];
    
    // ����ADC������
    if(adc_buffer_index < ADC_BUFFER_SIZE) {
        adc_buffer[adc_buffer_index++] = adc_value[0];
    } else {
        // �������������в��η���
        analyze_waveform();
        adc_buffer_index = 0;
    }

    // ������ڼ�¼�ҵ���ɼ����
    if (adc_recording_flag && (get_system_ms() - last_adc_time >= 100)) {
        last_adc_time = get_system_ms();

        // ׼�������ַ�����ADC����-ʱ���
        char data_buffer[64];
        uint32_t current_time = get_system_ms();
        sprintf(data_buffer, "%d-%lu\r\n", adc_value[0], current_time);

        // д���ļ�
        UINT bytes_written;
        FRESULT result = f_write(&adc_file, data_buffer, strlen(data_buffer), &bytes_written);
        if (result == FR_OK) {
            f_sync(&adc_file); // ����ͬ�����洢�豸
        } else {
            // д��ʧ�ܣ�ֹͣ��¼
            my_printf(DEBUG_USART, "ADC write error: %d, stopping recording\r\n", result);
            adc_stop_recording();
        }
    }
}

void analyze_waveform(void)
{
    // ÿ100ms����һ�β��η���
    if(get_system_ms() - last_analysis_time < 100) {
        return;
    }
    last_analysis_time = get_system_ms();

    // ����Ƶ�ʺͷ��ֵ
    float frequency = calculate_frequency_from_adc();
    float peak_to_peak = calculate_peak_to_peak_from_adc();

    // ����ȫ�ֲ��β���
    extern WaveformParams current_waveform;
    current_waveform.frequency = frequency;
    current_waveform.peak_to_peak = peak_to_peak;
}

float calculate_frequency_from_adc(void)
{
    // ʹ�ù����ⷽ������Ƶ��
    uint32_t crossings = 0;
    uint16_t prev_value = adc_buffer[0];
    uint16_t mid_point = 2048; // ADC�м�ֵ (4096/2)
    
    for(uint32_t i = 1; i < ADC_BUFFER_SIZE; i++) {
        if((prev_value < mid_point && adc_buffer[i] >= mid_point) ||
           (prev_value >= mid_point && adc_buffer[i] < mid_point)) {
            crossings++;
        }
        prev_value = adc_buffer[i];
    }
    
    // ����Ƶ��
    // ���������Ϊ100kHz (ÿ10usһ��������)
    float sampling_period = 0.00001f; // 10us
    float total_time = ADC_BUFFER_SIZE * sampling_period;
    float frequency = (float)crossings / (2.0f * total_time); // ����2����Ϊһ�����������ι���
    
    return frequency;
}

float calculate_peak_to_peak_from_adc(void)
{
    uint16_t max_val = 0;
    uint16_t min_val = 0xFFFF;
    
    for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++) {
        if(adc_buffer[i] > max_val) max_val = adc_buffer[i];
        if(adc_buffer[i] < min_val) min_val = adc_buffer[i];
    }
    
    // ��ADCֵת��Ϊ��ѹֵ
    float v_max = (float)max_val * 3.3f / 4096.0f;
    float v_min = (float)min_val * 3.3f / 4096.0f;
    
    return v_max - v_min;
}

void adc_start_recording(void)
{
    if (!adc_recording_flag) {
        // ȷ��֮ǰ���ļ��ѹر�
        f_close(&adc_file);

        // ����ļ�ϵͳ״̬
        DWORD free_clusters;
        FATFS *fs;
        FRESULT fs_result = f_getfree("0:", &free_clusters, &fs);
        if (fs_result != FR_OK) {
            my_printf(DEBUG_USART, "File system error: %d, trying to remount...\r\n", fs_result);
            // �������¹����ļ�ϵͳ
            f_mount(0, NULL); // ж��
            delay_ms(100);
            extern FATFS fs; // ʹ���ⲿ������ļ�ϵͳ����
            f_mount(0, &fs); // ���¹���
            delay_ms(100);
        } else {
            my_printf(DEBUG_USART, "File system OK, free clusters: %lu\r\n", free_clusters);
        }

        // �����ļ��������°���ʱ��ʱ���
        uint32_t timestamp = get_system_ms();
        sprintf(adc_filename, "0:/ADC_%lu.txt", timestamp);

        // ����ļ��Ƿ��Ѵ���
        FIL test_file;
        FRESULT test_result = f_open(&test_file, adc_filename, FA_READ);
        if (test_result == FR_OK) {
            f_close(&test_file);
            my_printf(DEBUG_USART, "File exists, deleting: %s\r\n", adc_filename);
            f_unlink(adc_filename); // ɾ���Ѵ��ڵ��ļ�
        }

        // ���������ļ�
        FRESULT result = f_open(&adc_file, adc_filename, FA_CREATE_ALWAYS | FA_WRITE);
        if (result == FR_OK) {
            adc_recording_flag = 1;
            last_adc_time = get_system_ms();
            my_printf(DEBUG_USART, "ADC recording started: %s\r\n", adc_filename);
        } else {
            my_printf(DEBUG_USART, "Failed to create ADC file: %s (Error: %d)\r\n", adc_filename, result);
            // ����ʹ�ü��ļ���
            sprintf(adc_filename, "0:/ADC.txt");
            result = f_open(&adc_file, adc_filename, FA_CREATE_ALWAYS | FA_WRITE);
            if (result == FR_OK) {
                adc_recording_flag = 1;
                last_adc_time = get_system_ms();
                my_printf(DEBUG_USART, "ADC recording started with fallback name: %s\r\n", adc_filename);
            } else {
                my_printf(DEBUG_USART, "Fallback file creation also failed (Error: %d)\r\n", result);
            }
        }
    } else {
        my_printf(DEBUG_USART, "ADC recording already in progress\r\n");
    }
}

void adc_stop_recording(void)
{
    if (adc_recording_flag) {
        f_sync(&adc_file); // ȷ������д��洢�豸
        f_close(&adc_file); // �ر��ļ�
        adc_recording_flag = 0;
        my_printf(DEBUG_USART, "ADC recording stopped: %s\r\n", adc_filename); // ������Ϣ
    } else {
        my_printf(DEBUG_USART, "No ADC recording in progress\r\n"); // ״̬��ʾ
    }
}

void adc_recording_init(void)
{
    // ��ʼ��ȫ�ֱ���
    adc_recording_flag = 0;
    memset(adc_filename, 0, sizeof(adc_filename));
    last_adc_time = 0;

    // ȷ���ļ�������ڹر�״̬
    f_close(&adc_file);

    // ����ļ�ϵͳ״̬
    adc_check_filesystem();

    my_printf(DEBUG_USART, "ADC recording system initialized\r\n");
}

void adc_check_filesystem(void)
{
    DWORD free_clusters;
    FATFS *fs;
    FRESULT result = f_getfree("0:", &free_clusters, &fs);

    if (result == FR_OK) {
        my_printf(DEBUG_USART, "File system status: OK\r\n");
        my_printf(DEBUG_USART, "Free clusters: %lu\r\n", free_clusters);
        my_printf(DEBUG_USART, "Cluster size: %d bytes\r\n", fs->csize * 512);
        my_printf(DEBUG_USART, "Free space: %lu KB\r\n", (free_clusters * fs->csize) / 2);
    } else {
        my_printf(DEBUG_USART, "File system error: %d\r\n", result);

        // �������³�ʼ���ļ�ϵͳ
        extern FATFS fs; // ʹ��sd_app.c�ж����fs
        my_printf(DEBUG_USART, "Attempting to reinitialize file system...\r\n");
        f_mount(0, NULL); // ж��
        delay_ms(200);
        result = f_mount(0, &fs); // ���¹���
        if (result == FR_OK) {
            my_printf(DEBUG_USART, "File system reinitialized successfully\r\n");
        } else {
            my_printf(DEBUG_USART, "File system reinitialize failed: %d\r\n", result);
        }
    }
}

