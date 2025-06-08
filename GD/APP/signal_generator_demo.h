#ifndef __SIGNAL_GENERATOR_DEMO_H
#define __SIGNAL_GENERATOR_DEMO_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化信号发生器演示
 */
void signal_generator_demo_init(void);

/**
 * @brief 信号发生器演示任务（需要在主循环中调用）
 */
void signal_generator_demo_task(void);

/**
 * @brief 停止演示
 */
void signal_generator_demo_stop(void);

/**
 * @brief 获取演示运行状态
 * @retval uint8_t: 1-运行中, 0-已停止
 */
uint8_t signal_generator_demo_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_GENERATOR_DEMO_H */
