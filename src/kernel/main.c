#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "thread.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "memory.h"
#include "stdio.h"
#include "fs.h"
#include "dir.h"

void init(void);

/*负责初始化所有模块 */
int main(void) {
   put_str("I am kernel\n");
   init_all();
   while(1);
   return 0;
}

void init(void){
   uint32_t ret_pid = fork();
   if(ret_pid) {
      printf("I am father process, pid: %d, child pid is %d\n", getpid(), ret_pid);
   } else {
      printf("I am child process, pid: %d, ret pid is %d\n", getpid(), ret_pid);
   }
   while(1);
}
