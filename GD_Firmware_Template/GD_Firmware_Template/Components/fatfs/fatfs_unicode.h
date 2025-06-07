/* 
 * FATFS Unicode Support Header
 * Company: MCUSTUDIO
 * Author: Ahypnis
 * Version: V0.10
 * Time: 2025/06/05
 * Note: 为_USE_LFN=3配置提供必需的Unicode处理函数声明
 */

#ifndef FATFS_UNICODE_H
#define FATFS_UNICODE_H

#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Unicode处理函数声明 */
#if _USE_LFN
WCHAR ff_convert(WCHAR src, UINT dir);  // Unicode字符转换
WCHAR ff_wtoupper(WCHAR chr);           // Unicode字符转大写
#endif

/* 内存管理函数声明 */
#if _USE_LFN == 3
void* ff_memalloc(UINT size);           // 分配内存
void ff_memfree(void* mblock);          // 释放内存
#endif

#ifdef __cplusplus
}
#endif

#endif /* FATFS_UNICODE_H */
