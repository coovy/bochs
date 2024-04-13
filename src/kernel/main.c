#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "thread.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"

void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;

/*负责初始化所有模块 */
int main(void)
{
   put_str("I am kernel\n");
   init_all();
   thread_start("consumer_a", 31, k_thread_a, "AAA ");
   thread_start("consumer_b", 31, k_thread_b, "BBB ");
   process_execute(u_prog_a, "user_prog_a");
   process_execute(u_prog_b, "user_prog_b");
   intr_enable();
   while (1)
      ;
   return 0;
}
void k_thread_a(void *arg)
{
   char* para = arg;
   while(1) {
      console_put_str(" v_a:0x");
      console_put_int(test_var_a);
   }
}

void k_thread_b(void *arg)
{
   char* parb = arg;
   while(1) {
      console_put_str(" v_b:0x");
      console_put_int(test_var_b);
   }
}

void u_prog_a(void) {
   while(1) {
      test_var_a++;
   }
}

void u_prog_b(void) {
   while(1) {
      test_var_b++;
   }
}