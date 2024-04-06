#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "thread.h"

void k_thread_a(void *);

/*负责初始化所有模块 */
int main(void) {
   put_str("I am kernel\n");
   init_all();
   thread_start("K_thread_a", 31, k_thread_a, "argA");
   while(1);
   return 0;
}
void k_thread_a(void* arg){
   char *para = arg;
   while(1){
      put_str(para);
   }
}