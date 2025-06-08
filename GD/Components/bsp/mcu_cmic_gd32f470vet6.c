/* Licence
* Company: MCUSTUDIO
* Auther: Ahypnis.
* Version: V0.10
* Time: 2025/06/05
* Note:
*/
#include "mcu_cmic_gd32f470vet6.h"

/* OLED 命令和数据缓冲区 */
__IO uint8_t oled_cmd_buf[2] = {0x00, 0x00};  // 命令缓冲区：控制字节 + 命令
__IO uint8_t oled_data_buf[2] = {0x40, 0x00}; // 数据缓冲区：控制字节 + 数据

/* SPI1 DMA 相关缓冲区 */
uint8_t spi1_send_array[ARRAYSIZE] = {0};    // SPI1 DMA 发送缓冲区
uint8_t spi1_receive_array[ARRAYSIZE] = {0}; // SPI1 DMA 接收缓冲区

/* 串口DMA接收 相关缓冲区 */
uint8_t rxbuffer[256];

/* ADC 相关缓冲区 */
uint16_t adc_value[1];

/* DAC 输出 */
uint16_t convertarr[CONVERT_NUM] = {0};

void bsp_led_init(void)
{
    /* enable the led clock */
    rcu_periph_clock_enable(LED_CLK_PORT);
    /* configure led GPIO port */ 
    gpio_mode_set(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN | LED5_PIN | LED6_PIN);
    gpio_output_options_set(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN | LED5_PIN | LED6_PIN);

    GPIO_BC(LED_PORT) = LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN | LED5_PIN | LED6_PIN;
}

void bsp_btn_init(void)
{
    /* enable the led clock */
    rcu_periph_clock_enable(KEYB_CLK_PORT);
    rcu_periph_clock_enable(KEYE_CLK_PORT);
    rcu_periph_clock_enable(KEYA_CLK_PORT);
    
    /* configure led GPIO port */ 
    gpio_mode_set(KEYB_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY4_PIN | KEY5_PIN);
    gpio_mode_set(KEYE_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY6_PIN);
    gpio_mode_set(KEYA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEYW_PIN);
}

/*!
    \brief      configure USART
    \param[in]  none
    \param[out] none
    \retval     none
*/
void bsp_usart_init(void)
{
    dma_single_data_parameter_struct dma_init_struct;
    
    rcu_periph_clock_enable(RCU_DMA1);
    
    dma_deinit(DMA1, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)rxbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = 256;
    dma_init_struct.periph_addr = USART0_RDATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);
    
    /* configure DMA mode */
    dma_circulation_disable(DMA1, DMA_CH2);
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);
    /* enable DMA1 channel2 */
    dma_channel_enable(DMA1, DMA_CH2);
    
    /* enable GPIO clock */
    rcu_periph_clock_enable(USARTI_CLK_PORT);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART0);
    
    /* connect port to USARTx_Tx */
    gpio_af_set(USART_PORT, GPIO_AF_7, USART_TX);

    /* connect port to USARTx_Rx */
    gpio_af_set(USART_PORT, GPIO_AF_7, USART_RX);

    /* configure USART Tx as alternate function push-pull */
    gpio_mode_set(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART_TX);
    gpio_output_options_set(USART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART_TX);

    /* configure USART Rx as alternate function push-pull */
    gpio_mode_set(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART_RX);
    gpio_output_options_set(USART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART_RX);

    /* configure USART */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_enable(USART0);
    
    nvic_irq_enable(USART0_IRQn, 0, 0);
    
    usart_interrupt_enable(USART0, USART_INT_IDLE);
}

void bsp_oled_init(void)
{
    dma_single_data_parameter_struct dma_init_struct;
    /* enable GPIOB clock */
    rcu_periph_clock_enable(OLED_CLK_PORT);
    /* enable I2C0 clock */
    rcu_periph_clock_enable(RCU_I2C0);
    /* enable DMA0 clock */
    rcu_periph_clock_enable(RCU_DMA0);
    
    /* connect PB9 to I2C0_SDA */
    gpio_af_set(OLED_PORT, GPIO_AF_4, OLED_DAT_PIN);
    /* connect PB8 to I2C0_SCL */
    gpio_af_set(OLED_PORT, GPIO_AF_4, OLED_CLK_PIN);
    
    gpio_mode_set(OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, OLED_DAT_PIN);
    gpio_output_options_set(OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_DAT_PIN);
    gpio_mode_set(OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, OLED_CLK_PIN);
    gpio_output_options_set(OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_CLK_PIN);
    
    /* configure I2C0 clock */
    i2c_clock_config(I2C0, 400000, I2C_DTCY_2);
    /* configure I2C0 address */
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C0_OWN_ADDRESS7);
    /* enable I2C0 */
    i2c_enable(I2C0);
    /* enable acknowledge */
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);
    
    /* 初始化 DMA 通道用于 I2C0 发送 */
    dma_deinit(DMA0, DMA_CH6);
    
    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)oled_data_buf;  // 先将地址设为数据缓冲区
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = 2;  // 发送 2 个字节 (控制字节 + 数据/命令)
    dma_init_struct.periph_addr = I2C0_DATA_ADDRESS;  // I2C0 数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);
    
    /* 配置 DMA 模式 */
    dma_circulation_disable(DMA0, DMA_CH6);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI1);  // I2C0 TX 对应的子外设
}

void bsp_gd25qxx_init(void)
{
    rcu_periph_clock_enable(SPI_CLK_PORT);
    rcu_periph_clock_enable(RCU_SPI1);
    rcu_periph_clock_enable(RCU_DMA0);
    
    /* configure SPI1 GPIO */
    gpio_af_set(SPI_PORT, GPIO_AF_5, SPI_SCK | SPI_MISO | SPI_MOSI);
    gpio_mode_set(SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK | SPI_MISO | SPI_MOSI);
    gpio_output_options_set(SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK | SPI_MISO | SPI_MOSI);

    /* set SPI1_NSS as GPIO*/
    gpio_mode_set(SPI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_NSS);
    gpio_output_options_set(SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_NSS);
    
    spi_parameter_struct spi_init_struct;

    /* configure SPI1 parameter */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_8;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI1, &spi_init_struct);

    /* 初始化 SPI Flash */
    spi_flash_init();
}

void bsp_adc_init(void)
{
    rcu_periph_clock_enable(ADC1_CLK_PORT);

    rcu_periph_clock_enable(RCU_ADC0);
    
    rcu_periph_clock_enable(RCU_DMA1);
    
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    
    /* config the GPIO as analog mode */
    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC1_PIN);
    
    /* ADC_DMA_channel configuration */
    dma_single_data_parameter_struct dma_single_data_parameter;

    /* ADC DMA_channel configuration */
    dma_deinit(DMA1, DMA_CH0);

    /* initialize DMA single data mode */
    dma_single_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_single_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_single_data_parameter.memory0_addr = (uint32_t)(adc_value);
    dma_single_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_single_data_parameter.direction = DMA_PERIPH_TO_MEMORY;
    dma_single_data_parameter.number = 1;
    dma_single_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &dma_single_data_parameter);
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);

    /* enable DMA circulation mode */
    dma_circulation_enable(DMA1, DMA_CH0);

    /* enable DMA channel */
    dma_channel_enable(DMA1, DMA_CH0);
    
    /* ADC mode config */
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    /* ADC contineous function disable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan mode disable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1);
    /* ADC routine channel config */
    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_15);
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_ROUTINE_CHANNEL, ADC_EXTTRIG_ROUTINE_T0_CH0); 
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    /* ADC DMA function enable */
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);

    /* enable ADC interface */
    adc_enable(ADC0);
    /* wait for ADC stability */
    delay_1ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* enable ADC software trigger */
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
}

void timer5_config(void)
{
    timer_parameter_struct timer_initpara;

    /* TIMER deinitialize */
    timer_deinit(TIMER5);

    /* TIMER configuration */
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 239;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 99;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    /* initialize TIMER init parameter struct */
    timer_init(TIMER5, &timer_initpara);

    /* TIMER master mode output trigger source: Update event */
    timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);

    /* enable TIMER */
    timer_enable(TIMER5);
}

void bsp_dac_init(void)
{
    /* enable GPIOA clock */
    rcu_periph_clock_enable(DAC1_CLK_PORT);
    /* enable DMA clock */
    rcu_periph_clock_enable(RCU_DMA0);
    /* enable DAC clock */
    rcu_periph_clock_enable(RCU_DAC);
    /* enable TIMER clock */
    rcu_periph_clock_enable(RCU_TIMER5);
    
    /* configure PA4 as DAC output */
    gpio_mode_set(DAC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, DAC1_PIN);
    
    dma_single_data_parameter_struct dma_struct;
    /* clear all the interrupt flags */
    dma_flag_clear(DMA0, DMA_CH5, DMA_INTF_FEEIF);
    dma_flag_clear(DMA0, DMA_CH5, DMA_INTF_SDEIF);
    dma_flag_clear(DMA0, DMA_CH5, DMA_INTF_TAEIF);
    dma_flag_clear(DMA0, DMA_CH5, DMA_INTF_HTFIF);
    dma_flag_clear(DMA0, DMA_CH5, DMA_INTF_FTFIF);    /* configure the DMA0 channel 5 */
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI7);
    
    dma_struct.periph_addr         = DAC0_R12DH_ADDRESS;  // 使用12位右对齐数据寄存器
    dma_struct.memory0_addr        = (uint32_t)convertarr;
    dma_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_struct.number              = CONVERT_NUM;
    dma_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_struct);

    dma_channel_enable(DMA0, DMA_CH5);
    
    /* initialize DAC */
    dac_deinit(DAC0);
    /* DAC trigger config */
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T5_TRGO);
    /* DAC trigger enable */
    dac_trigger_enable(DAC0, DAC_OUT0);
    /* DAC wave mode config */
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);

    /* DAC enable */
    dac_enable(DAC0, DAC_OUT0);
    /* DAC DMA function enable */
    dac_dma_enable(DAC0, DAC_OUT0);
    
    timer5_config();
}

void bsp_timer0_init(void)
{
    /* enable TIMER0 clock */
    rcu_periph_clock_enable(RCU_TIMER0);

    /* TIMER0 configuration for ADC trigger */
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 239;  // 240MHz / 240 = 1MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 999;  // 1MHz / 1000 = 1kHz default
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(TIMER0, &timer_initpara);

    /* TIMER0 master mode output trigger source: Update event */
    timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_UPDATE);
}

void timer0_config(uint32_t frequency_hz)
{
    if (frequency_hz == 0) frequency_hz = 1000; // 默认1kHz

    /* 停止定时器 */
    timer_disable(TIMER0);

    /* 计算周期值 */
    uint32_t period = 1000000 / frequency_hz - 1; // 1MHz时钟
    if (period > 65535) period = 65535;
    if (period < 1) period = 1;

    /* 重新配置定时器 */
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 239;  // 240MHz / 240 = 1MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = period;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(TIMER0, &timer_initpara);

    /* TIMER0 master mode output trigger source: Update event */
    timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_UPDATE);
}
