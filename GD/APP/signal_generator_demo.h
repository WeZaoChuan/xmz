#ifndef __SIGNAL_GENERATOR_DEMO_H
#define __SIGNAL_GENERATOR_DEMO_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ��ʼ���źŷ�������ʾ
 */
void signal_generator_demo_init(void);

/**
 * @brief �źŷ�������ʾ������Ҫ����ѭ���е��ã�
 */
void signal_generator_demo_task(void);

/**
 * @brief ֹͣ��ʾ
 */
void signal_generator_demo_stop(void);

/**
 * @brief ��ȡ��ʾ����״̬
 * @retval uint8_t: 1-������, 0-��ֹͣ
 */
uint8_t signal_generator_demo_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_GENERATOR_DEMO_H */
