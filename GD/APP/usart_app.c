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


const char *pi_file = "0:/DATA/pi.bin";
float  pi = 3.143356f;
extern FATFS fs;

uint16_t array[5] = {10, 20, 30, 40, 50};
const char *array_file = "0:/DATA/array.bin";

typedef struct {
            uint32_t id;
            char name[12];
            float temperature;
            uint8_t status;
        } SensorData;
        
SensorData sensor[4] = {
{1234,"sensor_1",32.5,5},
{1235,"sensor_2",33.5,5},
{1236,"sensor_3",34.5,5},
{1237,"sensor_4",35.5,5},
};

const char *sensor_file = "0:/DATA/sensor.bin";
				
int my_printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[256];
    va_list arg;
    int len;
    // 初始化可变参数列表
    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);
    
    for(tx_count = 0; tx_count < len; tx_count++){
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
        usart_data_transmit(usart_periph, buffer[tx_count]);
    }
    
    return len;
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

void parse_uart_command(uint8_t *buffer)
{
	if (strncmp((char *)buffer , "open_tf", 7) == 0)
	{ 
		uint16_t k = 5;
    DSTATUS stat = 0;
    do
    {
     stat = disk_initialize(0); // 初始化SD卡
    } while((stat != 0) && (--k));
		my_printf(DEBUG_USART, "SD Card disk_initialize:%d\r\n", stat);
    f_mount(0, &fs); // 挂载文件系统
		my_printf(DEBUG_USART, "SD Card f_mount:%d\r\n", stat);
		
		
    if(RES_OK == stat)
     {        
        my_printf(DEBUG_USART, "\r\nSD Card Initialize Success!\r\n");
				if (sd_fatfs_save_data(sensor_file, sensor, sizeof(sensor), 3))
				{
					my_printf(DEBUG_USART, "\r\n");
				} else {
        my_printf(DEBUG_USART, "数据存储失败!\r\n");
        return;
        }
     }
  }
	
	if (strncmp((char *)buffer , "close_tf", 8) == 0)
	{ 
		SensorData read_sensor[4];
    uint32_t bytes_read = sd_fatfs_read_data(sensor_file, read_sensor, sizeof(read_sensor), 3);
    if (bytes_read == sizeof(sensor)) 
			{
				 for (int i = 0; i < 5; i++){
         my_printf(DEBUG_USART, "读取结果: ID=%lu, 名称=%s, 温度=%.1f, 状态=0x%02X\r\n",
                  read_sensor[i].id, read_sensor[i].name, read_sensor[i].temperature, read_sensor[i].status);
        //print_data(&read_sensor, sizeof(read_sensor), DATA_TYPE_BINARY); 
				}
			}
	 else{
        my_printf(DEBUG_USART, "数据读取失败!\r\n");
        return;
    }
		f_mount(0, NULL);
    my_printf(DEBUG_USART, "SD卡文件系统卸载\r\n");
	}
}
void uart_task(void)
{
    if(!rx_flag) return;
    my_printf(DEBUG_USART, "%s", uart_dma_buffer);
		parse_uart_command(uart_dma_buffer);
    memset(uart_dma_buffer, 0, 256);
}
