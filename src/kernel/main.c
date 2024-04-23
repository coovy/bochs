#include "print.h"
#include "interrupt.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "memory.h"
#include "stdio.h"
#include "fs.h"
#include "dir.h"
#include "assert.h"
#include "shell.h"

void init(void);

/*负责初始化所有模块 */
int main(void) {
   put_str("I am kernel\n");
   init_all();
   cls_screen();
   console_put_str("[coovy@localhost /]$ ");
   while(1);
   return 0;
}

void init(void){
   uint32_t ret_pid = fork();
   if(ret_pid) {
      printf("I am father process, pid: %d, child pid is %d\n", getpid(), ret_pid);
   } else {
      my_shell();
   }
   panic("init: should not be here");
   // while(1);
}
