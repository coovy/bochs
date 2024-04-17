#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "stdint.h"

typedef char* va_list;

#define va_start(ap, v) ap = (va_list)&v // 把ap指向第一个固定参数v
#define va_arg(ap, t) *((t *)(ap += 4))    // ap指向下一个参数并返回其值, 32位栈存储单位是4字节
#define va_end(ap) ap = NULL               // 清除ap

uint32_t printf(const char* str, ...);
uint32_t vsprintf(char* str, const char* format, va_list ap);
uint32_t sprintf(char* buf, const char* format, ...);
#endif
