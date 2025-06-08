/* Licence
* Company: MCUSTUDIO
* Auther: Ahypnis.
* Version: V0.10
* Time: 2025/06/05
* Note:
*/
#include "mcu_cmic_gd32f470vet6.h"

__IO uint8_t tx_count = 0;
__IO uint8_t rx_flag = 0;
uint8_t uart_dma_buffer[256] = {0};

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

    my_printf(DEBUG_USART, "%s", uart_dma_buffer);
    memset(uart_dma_buffer, 0, 256);
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
