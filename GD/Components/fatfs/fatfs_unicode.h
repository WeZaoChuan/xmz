/* 
 * FATFS Unicode Support Header
 * Company: MCUSTUDIO
 * Author: Ahypnis
 * Version: V0.10
 * Time: 2025/06/05
 * Note: Ϊ_USE_LFN=3�����ṩ�����Unicode����������
 */

#ifndef FATFS_UNICODE_H
#define FATFS_UNICODE_H

#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Unicode���������� */
#if _USE_LFN
WCHAR ff_convert(WCHAR src, UINT dir);  // Unicode�ַ�ת��
WCHAR ff_wtoupper(WCHAR chr);           // Unicode�ַ�ת��д
#endif

/* �ڴ���������� */
#if _USE_LFN == 3
void* ff_memalloc(UINT size);           // �����ڴ�
void ff_memfree(void* mblock);          // �ͷ��ڴ�
#endif

#ifdef __cplusplus
}
#endif

#endif /* FATFS_UNICODE_H */
