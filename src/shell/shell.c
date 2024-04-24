#include "shell.h"
#include "stdint.h"
#include "fs.h"
#include "file.h"
#include "syscall.h"
#include "stdio.h"
#include "global.h"
#include "string.h"
#include "assert.h"
#include "buildin_cmd.h"

#define MAX_ARG_NR 16	   // 包括命令名的情况下, 最多支持15个参数

char* argv[MAX_ARG_NR];
int32_t argc = -1;
/* 存储输入的命令 */
static char cmd_line[MAX_PATH_LEN] = {0};

char final_path[MAX_PATH_LEN] = {0};    // 用于清洗路径时的缓冲区 

/* 用来记录当前目录, 是当前目录的缓存, 执行cd命令时会更新 */
char cwd_cache[64] = {0};


/* 输出提示符 */
void print_prompt(void) {
   printf("[coovy@localhost %s]$ ", cwd_cache);
}

/* 从键盘缓冲区中最多读入count个字节到buf。*/
static void readline(char* buf, int32_t count) {
   assert(buf != NULL && count > 0);
   char* pos = buf;
   while (read(stdin_no, pos, 1) != -1 && (pos - buf) < count) { // 在不出错情况下, 直到找到回车符才返回
      switch (*pos) {
         /* 回车或换行符标志命令结束, 直接返回 */
         case '\n':
         case '\r':
            *pos = 0;   // 添加cmd_line的终止字符0
            putchar('\n');
            return;

         case '\b':
            if (buf[0] != '\b') {   // 阻止删除非本次输入的信息
               --pos;	   // 退回到缓冲区cmd_line中上一个字符
               // putchar('\b');
            }
            break;
         
          /* ctrl+l清屏 */
         case 'l' - 'a': 
            *pos = 0;
            clear();
            print_prompt();
            // printf("%s", buf);
            break;

         /* ctrl+u 清掉输入 */
         case 'u' - 'a':
            while (buf != pos) {
               // putchar('\b');
               *(pos--) = 0;
            }
            break;

         /* 非控制键则输出字符 */
         default:
            // putchar(*pos);
            pos++;
      }
   }
   printf("readline: can`t find enter_key in the cmd_line, max num of char is 128\n");
}

/* 解析字符串cmd_str中以token为分隔符的单词,将各单词的指针存入argv数组；
思路就是面对一个字符串，然后将每个应该token分隔的地方，将第一个token替换为0，
这样argv数组就能指向每一个命令字或者参数了
 */
static int32_t cmd_parse(char* cmd_str, char** argv, char token) {
   assert(cmd_str != NULL);
   int32_t arg_idx = 0;

   // 初始化argv
   while(arg_idx < MAX_ARG_NR) {
      argv[arg_idx] = NULL;
      arg_idx++;
   }

   char* next = cmd_str;
   int32_t argc = 0;
   /* 循环处理命令行字符串 */
   while(*next) {
      while(*next == token) {
	      next++;
      }

      if (*next == 0) {
	      break; 
      }
      argv[argc] = next;

     /* 处理命令字和参数 */
      while (*next && *next != token) {	  // 在字符串结束前找单词分隔符
	      next++;
      }

      /* 遇到token字符, 替换为0， 做为结束标志 */
      if (*next) {
	      *next++ = 0;
      }
   
      /* 避免argv数组访问越界, 参数过多则返回0 */
      if (argc > MAX_ARG_NR) {
	      return -1;
      }
      argc++;
   }
   return argc;
}

/* shell */
void my_shell(void) {
   cwd_cache[0] = '/';
   while (1) {
      print_prompt();
      memset(final_path, 0, MAX_PATH_LEN);
      memset(cmd_line, 0, MAX_PATH_LEN);
      readline(cmd_line, MAX_PATH_LEN);
      if (cmd_line[0] == 0) {
	      continue;
      }
      argc = -1;
      argc = cmd_parse(cmd_line, argv, ' ');
      if (argc == -1) {
         printf("num of arguments exceed %d\n", MAX_ARG_NR);
         continue;
      }
      if (!strcmp("ls", argv[0])) {
	      buildin_ls(argc, argv);
      } else if (!strcmp("cd", argv[0])) {
      if (buildin_cd(argc, argv) != NULL) {
         memset(cwd_cache, 0, MAX_PATH_LEN);
         strcpy(cwd_cache, final_path);
	   }
      } else if (!strcmp("pwd", argv[0])) {
	      buildin_pwd(argc, argv);
      } else if (!strcmp("ps", argv[0])) {
	      buildin_ps(argc, argv);
      } else if (!strcmp("clear", argv[0])) {
	      buildin_clear(argc, argv);
      } else if (!strcmp("mkdir", argv[0])){
	      buildin_mkdir(argc, argv);
      } else if (!strcmp("rmdir", argv[0])){
	      buildin_rmdir(argc, argv);
      } else if (!strcmp("rm", argv[0])) {
	      buildin_rm(argc, argv);
      } else {
         // 外部命令, 从磁盘上加载
         int32_t pid = fork();
         if (pid) {	   // 父进程
            /* 下面这个while必须要加上, 否则父进程一般情况下会比子进程先执行,
            因此会进行下一轮循环将findl_path(这些全局变量)清空, 这样子进程将无法从final_path中获得参数*/
            while(1);
         } else {	   // 子进程
            make_clear_abs_path(argv[0], final_path); // argv[0]就是文件路径
            argv[0] = final_path;
            /* 先判断下文件是否存在 */
            struct stat file_stat;
            memset(&file_stat, 0, sizeof(struct stat));
            if (stat(argv[0], &file_stat) == -1) {
               printf("my_shell: cannot access %s: No such file or directory\n", argv[0]);
            } else {
               execv(argv[0], argv);
            }
            while(1);
         }
      }
      int32_t arg_idx = 0;
      while(arg_idx < MAX_ARG_NR) {
         argv[arg_idx] = NULL;
         arg_idx++;
      }
   }
   panic("my_shell: should not be here");
}
