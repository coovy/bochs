#ifndef __KERNEL_SHELL_H
#define __KERNEL_SHELL_H
#include "fs.h"

extern char final_path[MAX_PATH_LEN];
void print_prompt(void);
void my_shell(void);
#endif
