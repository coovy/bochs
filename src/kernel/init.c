#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"		  // 用相对路径演示头文件包含
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"

/*负责初始化所有模块 */
void init_all() {
   put_str("init_all\n");
   idt_init();
   timer_init();
   mem_init();
   thread_init(); // 初始化线程相关结构
   console_init();
   keyboard_init();
   tss_init();
   syscall_init();
   intr_enable();
   ide_init();
}