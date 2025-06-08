# GD32F4xx ��������ģ��˵��

���ĵ�˵������GD��Ŀ��������������Ҫ����ģ�飬��Щģ��ο�HAL��Ŀ��ʵ�֣�ʹ��GD32��׼����п�����

## 1. DAC���η�����ģ�� (dac_app)

### ��������
- ֧�����Ҳ������������ǲ�����
- ������Ƶ�ʡ����ȡ���������
- DMA�����ĸ��������
- ADCͬ������
- ���׼/�е��׼ģʽ�л�

### ��ҪAPI
```c
// ��ʼ��DACӦ��
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv);

// ���ò�������
int dac_app_set_waveform(dac_waveform_t type);

// ����Ƶ�ʺͷ���
int dac_app_set_frequency(uint32_t freq_hz);
int dac_app_set_amplitude(uint16_t peak_amplitude_mv);

// ����/ֹͣ���
int dac_app_start(void);
int dac_app_stop(void);
```

### ʹ��ʾ��
```c
// ��ʼ����1kHz���Ҳ���1V����
dac_app_init(1000, 1000);

// �л�������
dac_app_set_waveform(WAVEFORM_SQUARE);

// �ı�Ƶ�ʵ�2kHz
dac_app_set_frequency(2000);
```

## 2. �߼�ADC����ģ�� (adc_app)

### ��������
- ���ֲ���ģʽ�����Ρ���������ʱ��������DMAѭ����
- �����ò���Ƶ��
- DMA�����ĸ��ٲɼ�
- ��ʱ����������
- ����ͳ�Ʒ�������

### ��ҪAPI
```c
// ��ʼ��ADC
int adc_app_init(const adc_config_t *config);

// ����/ֹͣ����
int adc_app_start_sampling(uint16_t samples);
int adc_app_stop_sampling(void);

// ��ȡ���ݺ�״̬
uint16_t adc_app_get_data(uint16_t *buffer, uint16_t size);
adc_state_t adc_app_get_state(void);

// ����ת����ͳ��
uint16_t adc_app_raw_to_mv(uint16_t raw_value);
int adc_app_get_statistics(const uint16_t *buffer, uint16_t size, 
                          uint16_t *min, uint16_t *max, float *mean);
```

### ʹ��ʾ��
```c
// ����ADC
adc_config_t config = {
    .mode = ADC_MODE_DMA_CIRCULAR,
    .sample_rate_hz = 10000,
    .buffer_size = 1024,
    .channel = 0
};

// ��ʼ������������
adc_app_init(&config);
adc_app_start_sampling(1024);

// ��ȡ����
uint16_t buffer[1024];
uint16_t samples = adc_app_get_data(buffer, 1024);
```

## 3. ���η�����ģ�� (waveform_analyzer_app)

### ��������
- FFTƵ�׷���
- ��������ʶ��DC�����Ҳ������������ǲ���
- Ƶ�ʡ����ȡ���λ����
- г�����������Ρ����г����
- RMS�����ֵ����ֵ����
- ��г��ʧ��(THD)�������(SNR)����

### ��ҪAPI
```c
// ��ʼ��������
int waveform_analyzer_init(uint32_t sample_rate);

// ��������
float Get_Waveform_Frequency(const uint16_t *adc_val_buffer, uint16_t buffer_size);
ADC_WaveformType Get_Waveform_Type(const uint16_t *adc_val_buffer, uint16_t buffer_size);
float Get_Waveform_Vpp(const uint16_t *adc_val_buffer, uint16_t buffer_size, float *mean, float *rms);

// FFT����
int Perform_FFT(const uint16_t *adc_val_buffer, uint16_t buffer_size);

// �ۺϷ���
WaveformInfo Get_Waveform_Info(const uint16_t *adc_val_buffer, uint16_t buffer_size);
```

### ʹ��ʾ��
```c
// ��ʼ��������
waveform_analyzer_init(10000);

// ����ADC����
uint16_t adc_data[256];
WaveformInfo info = Get_Waveform_Info(adc_data, 256);

printf("Ƶ��: %.2f Hz\n", info.frequency);
printf("��������: %s\n", GetWaveformTypeString(info.waveform_type));
printf("���ֵ: %.3f V\n", info.vpp);
printf("THD: %.2f%%\n", info.thd);
```

## 4. ��ʾ���� (signal_generator_demo)

### ����˵��
�ṩ��һ����������ʾ����չʾ���ʹ����������ģ�飺
- �Զ�ѭ����ʾ��ͬ��������
- ʵʱ���η���
- ������̬����

### ʹ�÷���
```c
// ��main�����г�ʼ��
signal_generator_demo_init();

// ����ѭ���е���
while(1) {
    signal_generator_demo_task();
    // ��������...
}
```

## 5. Ӳ������

### DAC����
- ʹ��DAC0ͨ��0 (PA4)
- TIMER5��Ϊ����Դ
- DMA0ͨ��5�������ݴ���

### ADC����
- ʹ��ADC0ͨ��10 (PC0)
- TIMER0��Ϊ����Դ
- DMA1ͨ��0�������ݴ���

### ��ʱ������
- TIMER5: DAC������ʱ��
- TIMER0: ADC������ʱ��

## 6. ����ͼ���

### �ļ��б�
�������ļ���Ҫ��ӵ���Ŀ�У�
```
GD/APP/dac_app.h
GD/APP/dac_app.c
GD/APP/waveform_analyzer_app.h
GD/APP/waveform_analyzer_app.c
GD/APP/signal_generator_demo.h
GD/APP/signal_generator_demo.c
```

### ͷ�ļ�����
����ͷ�ļ�������ӣ�
```c
#include "dac_app.h"
#include "waveform_analyzer_app.h"
```

### ������
- ��׼��ѧ�� (math.h)
- GD32F4xx��׼�����
- ���е�BSP�㺯��

## 7. ע������

1. **�ڴ�ʹ��**: FFT������Ҫ�϶�RAM��ע���ڴ����
2. **ʱ������**: ȷ��ϵͳʱ��������ȷ��Ӱ�춨ʱ������
3. **�ж����ȼ�**: DMA�ж����ȼ���Ҫ��������
4. **����������**: ��MCU�������ƣ���߲�����Լ100kHz
5. **���ȿ���**: 12λADC/DAC���ȣ�ע���������

## 8. ��չ����

1. ��Ӹ��ನ�����ͣ���ݲ��������ȣ�
2. ʵ�ָ����ӵ��˲��㷨
3. ���Ƶ����ʾ����
4. ֧�ֶ�ͨ��ͬ���ɼ�
5. ʵ�ֲ��δ洢�ͻطŹ���

---

**��Ȩ����**: ���������HAL��Ŀ�߼���ʹ��GD32F4xx��׼��ʵ�֣�������GD32F470VET6΢��������
