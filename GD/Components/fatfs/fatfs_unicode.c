/* 
 * FATFS Unicode Support Implementation
 * Company: MCUSTUDIO
 * Author: Ahypnis
 * Version: V0.10
 * Time: 2025/06/05
 * Note: 为_USE_LFN=3配置提供必需的Unicode处理函数
 */

#include "ff.h"
#include <stdlib.h>
#include <string.h>

#if _USE_LFN == 3

/* 内存分配函数 - 已在syscall.c中实现，这里提供备用实现 */
#ifndef FF_MEMALLOC_DEFINED
void* ff_memalloc(UINT size)
{
    return malloc(size);
}

void ff_memfree(void* mblock)
{
    free(mblock);
}
#define FF_MEMALLOC_DEFINED
#endif

#endif /* _USE_LFN == 3 */

#if _USE_LFN

/* 简化的Unicode转换表 - 支持基本ASCII和常用字符 */
static const WCHAR unicode_to_ascii_table[] = {
    /* 基本ASCII字符直接映射 */
    0x0020, 0x0020, 0x0021, 0x0021, 0x0022, 0x0022, 0x0023, 0x0023,
    0x0024, 0x0024, 0x0025, 0x0025, 0x0026, 0x0026, 0x0027, 0x0027,
    0x0028, 0x0028, 0x0029, 0x0029, 0x002A, 0x002A, 0x002B, 0x002B,
    0x002C, 0x002C, 0x002D, 0x002D, 0x002E, 0x002E, 0x002F, 0x002F,
    /* 数字 0-9 */
    0x0030, 0x0030, 0x0031, 0x0031, 0x0032, 0x0032, 0x0033, 0x0033,
    0x0034, 0x0034, 0x0035, 0x0035, 0x0036, 0x0036, 0x0037, 0x0037,
    0x0038, 0x0038, 0x0039, 0x0039,
    /* 结束标记 */
    0x0000, 0x0000
};

/* 大小写转换表 */
static const WCHAR lower_to_upper_table[] = {
    /* 小写字母 a-z 转换为大写 A-Z */
    0x0061, 0x0041, 0x0062, 0x0042, 0x0063, 0x0043, 0x0064, 0x0044,
    0x0065, 0x0045, 0x0066, 0x0046, 0x0067, 0x0047, 0x0068, 0x0048,
    0x0069, 0x0049, 0x006A, 0x004A, 0x006B, 0x004B, 0x006C, 0x004C,
    0x006D, 0x004D, 0x006E, 0x004E, 0x006F, 0x004F, 0x0070, 0x0050,
    0x0071, 0x0051, 0x0072, 0x0052, 0x0073, 0x0053, 0x0074, 0x0054,
    0x0075, 0x0055, 0x0076, 0x0056, 0x0077, 0x0057, 0x0078, 0x0058,
    0x0079, 0x0059, 0x007A, 0x005A,
    /* 结束标记 */
    0x0000, 0x0000
};

/**
 * @brief Unicode字符转换函数
 * @param src 源字符
 * @param dir 转换方向：0=Unicode到OEM，1=OEM到Unicode
 * @return 转换后的字符，0表示转换失败
 */
WCHAR ff_convert(WCHAR src, UINT dir)
{
    WCHAR result = 0;
    
    /* ASCII字符范围直接返回 */
    if (src <= 0x007F) {
        return src;
    }
    
    /* 对于扩展字符，进行简化处理 */
    if (dir == 0) {
        /* Unicode到OEM：简化为ASCII兼容字符 */
        if (src >= 0x00C0 && src <= 0x00FF) {
            /* 拉丁字符简化处理 */
            switch (src) {
                case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3: 
                case 0x00C4: case 0x00C5: result = 'A'; break;
                case 0x00C7: result = 'C'; break;
                case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB: result = 'E'; break;
                case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF: result = 'I'; break;
                case 0x00D1: result = 'N'; break;
                case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5: 
                case 0x00D6: case 0x00D8: result = 'O'; break;
                case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC: result = 'U'; break;
                case 0x00DD: result = 'Y'; break;
                case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: 
                case 0x00E4: case 0x00E5: result = 'a'; break;
                case 0x00E7: result = 'c'; break;
                case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: result = 'e'; break;
                case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: result = 'i'; break;
                case 0x00F1: result = 'n'; break;
                case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: 
                case 0x00F6: case 0x00F8: result = 'o'; break;
                case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: result = 'u'; break;
                case 0x00FD: case 0x00FF: result = 'y'; break;
                default: result = '_'; break; // 未知字符用下划线替代
            }
        } else {
            result = '_'; // 其他Unicode字符用下划线替代
        }
    } else {
        /* OEM到Unicode：简单映射 */
        result = src;
    }
    
    return result;
}

/**
 * @brief Unicode字符转大写函数
 * @param chr 输入字符
 * @return 转换为大写的字符
 */
WCHAR ff_wtoupper(WCHAR chr)
{
    /* ASCII范围的小写字母转大写 */
    if (chr >= 0x0061 && chr <= 0x007A) {
        return chr - 0x0020; // 'a'-'z' 转换为 'A'-'Z'
    }
    
    /* 查找转换表 */
    for (int i = 0; lower_to_upper_table[i] != 0; i += 2) {
        if (chr == lower_to_upper_table[i]) {
            return lower_to_upper_table[i + 1];
        }
    }
    
    /* 扩展Unicode字符的简化处理 */
    if (chr >= 0x00E0 && chr <= 0x00FF) {
        /* 拉丁小写字母转大写 */
        switch (chr) {
            case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: 
            case 0x00E4: case 0x00E5: return 0x00C0 + (chr - 0x00E0);
            case 0x00E7: return 0x00C7;
            case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: 
                return 0x00C8 + (chr - 0x00E8);
            case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: 
                return 0x00CC + (chr - 0x00EC);
            case 0x00F1: return 0x00D1;
            case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: 
            case 0x00F6: return 0x00D2 + (chr - 0x00F2);
            case 0x00F8: return 0x00D8;
            case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: 
                return 0x00D9 + (chr - 0x00F9);
            case 0x00FD: case 0x00FF: return 0x00DD;
        }
    }
    
    /* 无需转换，返回原字符 */
    return chr;
}

#endif /* _USE_LFN */
