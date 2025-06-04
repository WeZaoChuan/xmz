#include "oled_app.h"
 
// 修改为extern，因为状态变量定义在btn_app.c中
extern volatile uint8_t oled_display_state; // 0:WuZhaowei:19, 1:Zhoujing, 2:3638, 3:1 5 1
extern volatile uint8_t oled_update_flag;   // 更新标志
/**
 * @brief	使用类似printf的方式显示字符串，显示6x8大小的ASCII字符
 * @param x  Character position on the X-axis  range：0 - 127
 * @param y  Character position on the Y-axis  range：0 - 3
 * 例如：oled_printf(0, 0, "Data = %d", dat);
 **/
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512]; // 临时存储格式化后的字符串
  va_list arg;      // 处理可变参数
  int len;          // 最终字符串长度

  va_start(arg, format);
  // 安全地格式化字符串到 buffer
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

// u8g2 的 GPIO 和延时回调函数
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch (msg)
  {
  case U8X8_MSG_GPIO_AND_DELAY_INIT:
    // 初始化 GPIO (如果需要，例如 SPI 的 CS, DC, RST 引脚)
    // 对于硬件 I2C，这里通常不需要做什么
    break;
  case U8X8_MSG_DELAY_MILLI:
    // 原因: u8g2 内部某些操作需要毫秒级的延时等待。
    // 提供毫秒级延时，直接调用 HAL 库函数。
    HAL_Delay(arg_int);
    break;
  case U8X8_MSG_DELAY_10MICRO:
    // 实现10微秒延时，使用精确校准的空循环
    {
      // GD32系列通常运行速度为120-200MHz，每个循环大约需要3-4个时钟周期
      // 按160MHz计算，10μs需要约400-500个循环
      for (volatile uint32_t i = 0; i < 480; i++)
      {
        __NOP(); // 编译器不会优化掉这个指令
      }
    }
    break;
  case U8X8_MSG_DELAY_100NANO:
    // 实现100纳秒延时，使用多个NOP指令
    // 每个NOP指令大约需要1个时钟周期(约6ns@160MHz)
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    break;
  case U8X8_MSG_GPIO_I2C_CLOCK: // [[fallthrough]] // Fallthrough 注释表示有意为之
  case U8X8_MSG_GPIO_I2C_DATA:
    // 控制 SCL/SDA 引脚电平。这些仅在**软件模拟 I2C** 时需要实现。
    // 使用硬件 I2C 时，这些消息可以忽略，由 HAL 库处理。
    break;
  // --- 以下是 GPIO 相关的消息，主要用于按键输入或 SPI 控制 ---
  // 如果你的 u8g2 应用需要读取按键或控制 SPI 引脚 (CS, DC, Reset)，
  // 你需要在这里根据 msg 类型读取/设置对应的 GPIO 引脚状态。
  // 对于仅使用硬件 I2C 显示的场景，可以像下面这样简单返回不支持。
  case U8X8_MSG_GPIO_CS:
    // SPI 片选控制
    break;
  case U8X8_MSG_GPIO_DC:
    // SPI 数据/命令线控制
    break;
  case U8X8_MSG_GPIO_RESET:
    // 显示屏复位引脚控制
    break;
  case U8X8_MSG_GPIO_MENU_SELECT:
    u8x8_SetGPIOResult(u8x8, /* 读取选择键 GPIO 状态 */ 0);
    break;
  default:
    u8x8_SetGPIOResult(u8x8, 1); // 不支持的消息
    break;
  }
  return 1;
}

// u8g2 的硬件 I2C 通信回调函数
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t buffer[32]; // u8g2 每次传输最大 32 字节
  static uint8_t buf_idx;
  uint8_t *data;

  switch (msg)
  {
  case U8X8_MSG_BYTE_SEND:
    // 原因: u8g2 通常不会一次性发送大量数据，而是分块发送。
    // 这个消息用于将一小块数据 (arg_int 字节) 从 u8g2 内部传递到我们的回调函数。
    // 我们需要将这些数据暂存到本地 buffer 中，等待 START/END_TRANSFER 信号。
    data = (uint8_t *)arg_ptr;
    while (arg_int > 0)
    {
      buffer[buf_idx++] = *data;
      data++;
      arg_int--;
    }
    break;
  case U8X8_MSG_BYTE_INIT:
    // 原因: 提供一个机会进行 I2C 外设的初始化。
    // 初始化 I2C (通常在 main 函数中已完成)
    // 由于我们在 main 函数中已经调用了 MX_I2C1_Init()，这里通常可以留空。
    break;
  case U8X8_MSG_BYTE_SET_DC:
    // 原因: 这个消息用于 SPI 通信中控制 Data/Command 选择引脚。
    // 设置数据/命令线 (I2C 不需要)
    // I2C 通过特定的控制字节 (0x00 或 0x40) 区分命令和数据，因此该消息对于 I2C 无意义。
    break;
  case U8X8_MSG_BYTE_START_TRANSFER:
    // 原因: 标记一个 I2C 传输序列的开始。
    buf_idx = 0;
    // 我们在这里重置本地缓冲区的索引，准备接收新的数据块。
    break;
  case U8X8_MSG_BYTE_END_TRANSFER:
    // 原因: 标记一个 I2C 传输序列的结束。
    // 此时，本地 buffer 中已经暂存了所有需要发送的数据块。
    // 这是执行实际 I2C 发送操作的最佳时机。
    // 发送缓冲区中的数据
    // 注意: u8x8_GetI2CAddress(u8x8) 返回的是 7 位地址 * 2 = 8 位地址
    if (HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 100) != HAL_OK)
    {
      return 0; // 发送失败
    }
    break;
  default:
    return 0;
  }
  return 1;
}

/* 缓存刷新函数 */
void OLED_SendBuff(uint8_t buff[4][128])
{
  // 获取 u8g2 的缓冲区指针
  uint8_t *u8g2_buffer = u8g2_GetBufferPtr(&u8g2);

  // 将数据拷贝到 u8g2 的缓冲区
  memcpy(u8g2_buffer, buff, 4 * 128);

  // 发送整个缓冲区到 OLED
  u8g2_SendBuffer(&u8g2);
}

 

/* Oled 显示任务 */
void oled_task(void)
{
  //  // --- 准备阶段 ---
  //  // 设置绘图颜色 (对于单色屏，1 通常表示点亮像素)
  //  u8g2_SetDrawColor(&u8g2, 1);
  //  // 选择要使用的字体 (确保字体文件已添加到工程)
  //  u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr); // ncenB08: 字体名, _tr: 透明背景

  //  // --- 核心绘图流程 ---
  //  // 1. 清除内存缓冲区 (非常重要，每次绘制新帧前必须调用)
  //  u8g2_ClearBuffer(&u8g2);

  //  // 2. 使用 u8g2 API 在缓冲区中绘图
  //  //    所有绘图操作都作用于 RAM 中的缓冲区。
  //  // 绘制字符串 (参数: u8g2实例, x坐标, y坐标, 字符串)
  //  // y 坐标通常是字符串基线的位置。
  //  u8g2_DrawStr(&u8g2, 2, 12, "Hello u8g2!"); // 从 (2, 12) 开始绘制
  //  u8g2_DrawStr(&u8g2, 2, 28, "Micron Elec Studio"); // 绘制第二行

  //  // 绘制图形 (示例：一个空心圆和一个实心框)
  //  // 绘制圆 (参数: u8g2实例, 圆心x, 圆心y, 半径, 绘制选项)
  //  u8g2_DrawCircle(&u8g2, 90, 19, 10, U8G2_DRAW_ALL); // U8G2_DRAW_ALL 画圆周
  //  // 绘制实心框 (参数: u8g2实例, 左上角x, 左上角y, 宽度, 高度)
  //  // u8g2_DrawBox(&u8g2, 50, 15, 20, 10);
  //  // 绘制空心框 (参数: u8g2实例, 左上角x, 左上角y, 宽度, 高度)
  //  // u8g2_DrawFrame(&u8g2, 50, 15, 20, 10);

  //  // 3. 将缓冲区内容一次性发送到屏幕 (非常重要)
  //  //    这个函数会调用我们之前编写的 I2C 回调函数，将整个缓冲区的数据发送出去。
  //  u8g2_SendBuffer(&u8g2);
 
    static uint8_t last_state = 0xFF; // 初始值设为不可能的值
    
    // 只在需要更新或状态改变时才刷新
    if(oled_update_flag || (last_state != oled_display_state))
    {
        OLED_Clear();
        
        switch(oled_display_state)
        {
            case 0:
                oled_printf(0, 0, "WuZhaowei:19");  // 显示WuZhaowei:19
						    oled_printf(0, 1, "Wuwei:19");  // 显示WuZhaowei:19
						

                break;
            case 1:
                oled_printf(32, 1, "西门子");      // 显示Zhoujing
                break;
            case 2:
                oled_printf(32, 1, "3638");          // 显示3638
                break;
            case 3:
                oled_printf(32, 1, "1 5 1");         // 显示1 5 1
                break;
            default:
                break;
        }
        
        last_state = oled_display_state;
        oled_update_flag = 0; // 清除更新标志
    
}
  
}
