/* Licence
* Company: MCUSTUDIO
* Auther: Ahypnis.
* Version: V0.10
* Time: 2025/06/05
* Note:
*/
#include "mcu_cmic_gd32f470vet6.h"

// 波形类型定义
typedef enum {
    WAVEFORM_SINE = 0,
    WAVEFORM_SQUARE = 1,
    WAVEFORM_TRIANGLE = 2,
    WAVEFORM_SAWTOOTH = 3
} WaveformType;

// 波形参数结构体
typedef struct {
    WaveformType type;
    float frequency;
    float peak_to_peak;
} WaveformParams;

// 全局变量
__IO uint8_t tx_count = 0;
__IO uint8_t rx_flag = 0;
uint8_t uart_dma_buffer[256] = {0};
WaveformParams current_waveform = {
    .type = WAVEFORM_SINE,
    .frequency = 1000.0f,
    .peak_to_peak = 3.3f
};

// 函数声明
void process_command(char* cmd);
void handle_query_command(char* param);
void handle_set_command(char* param, char* value);
float calculate_frequency(uint16_t* adc_data, uint32_t data_count);
float calculate_peak_to_peak(uint16_t* adc_data, uint32_t data_count);

int my_printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[256];
    va_list arg;
    int len;
    // ????????????б?
    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);
    
    for(tx_count = 0; tx_count < len; tx_count++){
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
        usart_data_transmit(usart_periph, buffer[tx_count]);
    }
    
    return len;
}

void uart_task(void)
{
    if(!rx_flag) return;

    // 处理接收到的命令
    process_command((char*)uart_dma_buffer);
    
    // 清空接收缓冲区
    memset(uart_dma_buffer, 0, 256);
    rx_flag = 0;
}

void process_command(char* cmd)
{
    char* token;
    char* param;
    char* value;
    
    // 移除命令字符串末尾的换行符
    char* newline = strchr(cmd, '\r');
    if(newline) *newline = '\0';
    newline = strchr(cmd, '\n');
    if(newline) *newline = '\0';
    
    // 分割命令和参数
    token = strtok(cmd, " ");
    if(token == NULL) return;
    
    if(strcmp(token, "QUERY") == 0) {
        param = strtok(NULL, " ");
        if(param) {
            handle_query_command(param);
        } else {
            my_printf(DEBUG_USART, "Error: Missing parameter for QUERY command\r\n");
        }
    }
    else if(strcmp(token, "SET") == 0) {
        param = strtok(NULL, " ");
        value = strtok(NULL, " ");
        if(param && value) {
            handle_set_command(param, value);
        } else {
            my_printf(DEBUG_USART, "Error: Missing parameter or value for SET command\r\n");
        }
    }
    else {
        my_printf(DEBUG_USART, "Unknown command: %s\r\n", token);
    }
}

void handle_query_command(char* param)
{
    if(strcmp(param, "WAVEFORM") == 0) {
        const char* type_str[] = {"SINE", "SQUARE", "TRIANGLE", "SAWTOOTH"};
        my_printf(DEBUG_USART, "Current waveform type: %s\r\n", type_str[current_waveform.type]);
    }
    else if(strcmp(param, "FREQUENCY") == 0) {
        float freq = calculate_frequency(convertarr, CONVERT_NUM);
        my_printf(DEBUG_USART, "Current frequency: %.2f Hz\r\n", freq);
    }
    else if(strcmp(param, "PEAK2PEAK") == 0) {
        float p2p = calculate_peak_to_peak(convertarr, CONVERT_NUM);
        my_printf(DEBUG_USART, "Current peak-to-peak voltage: %.2f V\r\n", p2p);
    }
    else {
        my_printf(DEBUG_USART, "Unknown parameter: %s\r\n", param);
    }
}

void handle_set_command(char* param, char* value)
{
    if(strcmp(param, "WAVEFORM") == 0) {
        if(strcmp(value, "SINE") == 0) current_waveform.type = WAVEFORM_SINE;
        else if(strcmp(value, "SQUARE") == 0) current_waveform.type = WAVEFORM_SQUARE;
        else if(strcmp(value, "TRIANGLE") == 0) current_waveform.type = WAVEFORM_TRIANGLE;
        else if(strcmp(value, "SAWTOOTH") == 0) current_waveform.type = WAVEFORM_SAWTOOTH;
        else {
            my_printf(DEBUG_USART, "Invalid waveform type: %s\r\n", value);
            return;
        }
        my_printf(DEBUG_USART, "Waveform type set to: %s\r\n", value);
    }
    else if(strcmp(param, "FREQUENCY") == 0) {
        float freq = atof(value);
        if(freq > 0 && freq <= 100000) {
            current_waveform.frequency = freq;
            my_printf(DEBUG_USART, "Frequency set to: %.2f Hz\r\n", freq);
        } else {
            my_printf(DEBUG_USART, "Invalid frequency value: %s\r\n", value);
        }
    }
    else if(strcmp(param, "PEAK2PEAK") == 0) {
        float p2p = atof(value);
        if(p2p > 0 && p2p <= 3.3) {
            current_waveform.peak_to_peak = p2p;
            my_printf(DEBUG_USART, "Peak-to-peak voltage set to: %.2f V\r\n", p2p);
        } else {
            my_printf(DEBUG_USART, "Invalid peak-to-peak value: %s\r\n", value);
        }
    }
    else {
        my_printf(DEBUG_USART, "Unknown parameter: %s\r\n", param);
    }
}

float calculate_frequency(uint16_t* adc_data, uint32_t data_count)
{
    // 实现频率计算算法
    // 这里需要根据实际采样数据进行过零检测或FFT分析
    // 返回计算得到的频率值
    return current_waveform.frequency; // 临时返回设置值
}

float calculate_peak_to_peak(uint16_t* adc_data, uint32_t data_count)
{
    // 实现峰峰值计算算法
    // 查找最大值和最小值，计算差值
    uint16_t max_val = 0;
    uint16_t min_val = 0xFFFF;
    
    for(uint32_t i = 0; i < data_count; i++) {
        if(adc_data[i] > max_val) max_val = adc_data[i];
        if(adc_data[i] < min_val) min_val = adc_data[i];
    }
    
    // 将ADC值转换为电压值
    float v_max = (float)max_val * 3.3f / 4096.0f;
    float v_min = (float)min_val * 3.3f / 4096.0f;
    
    return v_max - v_min;
}

void print_adc_file_info(void)
{
    extern char adc_filename[32];
    extern uint8_t adc_recording_flag;

    if (strlen(adc_filename) == 0) {
        my_printf(DEBUG_USART, "No ADC file recorded yet.\r\n");
        return;
    }

    // 如果正在记录，先提示用户
    if (adc_recording_flag) {
        my_printf(DEBUG_USART, "Warning: ADC recording is still active\r\n");
    }

    FIL file;
    FRESULT result = f_open(&file, adc_filename, FA_READ);
    if (result != FR_OK) {
        my_printf(DEBUG_USART, "Failed to open ADC file: %s (Error: %d)\r\n", adc_filename, result);
        return;
    }

    // 获取文件大小
    DWORD file_size = f_size(&file);
    my_printf(DEBUG_USART, "ADC File: %s\r\n", adc_filename);
    my_printf(DEBUG_USART, "File Size: %lu bytes\r\n", file_size);
    my_printf(DEBUG_USART, "File Content:\r\n");

    // 读取并打印文件内容
    char read_buffer[128];
    UINT bytes_read;

    while (1) {
        result = f_read(&file, read_buffer, sizeof(read_buffer) - 1, &bytes_read);
        if (result != FR_OK || bytes_read == 0) break;

        read_buffer[bytes_read] = '\0'; // 确保字符串结束
        my_printf(DEBUG_USART, "%s", read_buffer);
    }

    f_close(&file);
    my_printf(DEBUG_USART, "\r\n--- End of File ---\r\n");
}
