#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "thread.h"
#include "console.h"

#include "ioqueue.h"
#include "keyboard.h"

void k_thread_a(void *);
void k_thread_b(void *);
/*负责初始化所有模块 */
int main(void)
{
   put_str("I am kernel\n");
   init_all();
   thread_start("consumer_a", 31, k_thread_a, "AAA ");
   thread_start("consumer_b", 31, k_thread_b, "BBB ");
   intr_enable();
   while (1)
      ;
   return 0;
}
void k_thread_a(void *arg)
{
   // char *para = arg;
   // while(1){
   //    put_str(para);
   // }
   while (1)
   {
      enum intr_status old_status = intr_disable();
      if (!ioq_empty(&kbd_buf))
      {
         console_put_str(arg);
         console_put_char(' ');
         char byte = ioq_getchar(&kbd_buf);
         console_put_char(byte);
         console_put_char(' ');
      }
      intr_set_status(old_status);
   }
}

void k_thread_b(void *arg)
{
   // char* para = arg;
   // while(1) {
   //    put_str(para);
   // }
   while (1)
   {
      enum intr_status old_status = intr_disable();
      if (!ioq_empty(&kbd_buf))
      {
         console_put_str(arg);
         console_put_char(' ');
         char byte = ioq_getchar(&kbd_buf);
         console_put_char(byte);
         console_put_char(' ');
      }
      intr_set_status(old_status);
   }
}