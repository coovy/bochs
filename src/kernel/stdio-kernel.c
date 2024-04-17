#include "stdio-kernel.h"
#include "stdint.h"
#include "stdio.h"
#include "console.h"
#include "global.h"

void printk(const char* format, ...){
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    console_put_str(buf);       // 用户是系统调用write，内核实现这个函数节约资源
}