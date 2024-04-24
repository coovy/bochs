#include "buildin_cmd.h"
#include "syscall.h"
#include "stdio.h"
#include "string.h"
#include "fs.h"
#include "global.h"
#include "dir.h"
#include "shell.h"
#include "assert.h"

/* 将路径old_abs_path中的..和.转换为实际路径后存入new_abs_path，
不过仅仅是简单的处理，因为assert(old_abs_path[0] == '/'); 是对路径的清洗*/
static void wash_path(char* old_abs_path, char* new_abs_path) {
   assert(old_abs_path[0] == '/');
   char name[MAX_FILE_NAME_LEN] = {0};    
   char* sub_path = old_abs_path;
   sub_path = path_parse(sub_path, name);
   if (name[0] == 0) { // 说明只是输入了'/' 
      new_abs_path[0] = '/';
      new_abs_path[1] = 0;
      return;
   }
   new_abs_path[0] = 0;	   // 保证缓冲区缓冲区干净
   strcat(new_abs_path, "/");
   while (name[0]) {
        if (!strcmp("..", name)) {
            // 如果是".."
	        char* slash_ptr = strrchr(new_abs_path, '/'); // 找到最后一个斜杠的位置

            // 将
            if (slash_ptr != new_abs_path) {
                // 如果这个斜杠不是开始的斜杠，那么就将最后一个斜杠"以及"后边的字符串去掉，相当于返回上一层目录，"/a/b"->"/a"
                *slash_ptr = 0;
            } else {
                // "/a"->"/"
                *(slash_ptr + 1) = 0;
            }
        } else if (strcmp(".", name)) {
            // 如果这层路径名称不是'.', 拼接就行
            if (strcmp(new_abs_path, "/")) {
                strcat(new_abs_path, "/");
            }
            strcat(new_abs_path, name);
        }
        // '.'呢？，不用处理，相当于直接丢弃

        /* 继续遍历下一层路径 */
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (sub_path) {
            sub_path = path_parse(sub_path, name);
        }
    }
}

/* 将path处理成不含..和.的绝对路径,存储在final_path */
void make_clear_abs_path(char* path, char* final_path) {
    char abs_path[MAX_PATH_LEN] = {0};
    if (path[0] != '/') {
        memset(abs_path, 0, MAX_PATH_LEN);
        if (getcwd(abs_path, MAX_PATH_LEN) != NULL) {
            if (!((abs_path[0] == '/') && (abs_path[1] == 0))) {
                // 如果此时获取的abs_path不是根目录，那就拼接'/'再清洗
                strcat(abs_path, "/");
            }
        }
    }
   strcat(abs_path, path);
   wash_path(abs_path, final_path);
}

void buildin_pwd(uint32_t argc, char** argv UNUSED) {
    if (argc != 1) {
        printf("pwd: no argument support!\n");
        return;
    } else {
        if (NULL != getcwd(final_path, MAX_PATH_LEN)) {
            printf("%s\n", final_path); 
        } else {
            printf("pwd: get current work directory failed.\n");
        }
    }
}

char* buildin_cd(uint32_t argc, char** argv) {
    if (argc > 2) {
        printf("cd: only support 1 argument!\n");
        return NULL;
    }

    /* 不指定cd的目录(无参数) */
    if (argc == 1) {
        final_path[0] = '/';
        final_path[1] = 0;
    } else {
        // 指定cd的目录
        make_clear_abs_path(argv[1], final_path);
    }

    if (chdir(final_path) == -1) {
        printf("cd: no such directory %s\n", final_path);
        return NULL;
    }
    return final_path;
}

void buildin_ls(uint32_t argc, char** argv) {
    char* pathname = NULL;
    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));
    bool long_info = false;
    uint32_t arg_path_nr = 0; // 相当于参数部分的编号数量，做个flag，ls仅仅接收一个参数
    uint32_t arg_idx = 1;   // argv[0]是命令本身，这里是"ls"
    while (arg_idx < argc) {
        if (argv[arg_idx][0] == '-') {	  // argv[arg_idx]指向的是字符串。若是参数选项, 则是以'-'开头
        if (!strcmp("-l", argv[arg_idx])) {         // 参数-l
            long_info = true;
        } else if (!strcmp("-h", argv[arg_idx])) {   // 参数-h
            printf("usage: -l list all infomation about the file.\n-h for help\nlist all files in the current dirctory if no option.\n"); 
            return;
        } else {	// 其他参数不支持
            printf("ls: invalid option %s\nTry `ls -h' for more information.\n", argv[arg_idx]);
            return;
        }
        } else {    // ls的路径参数
            if (arg_path_nr == 0) {
                pathname = argv[arg_idx];
                arg_path_nr = 1;
            } else {
                printf("ls: only support one path\n");
                return;
            }
        }
        arg_idx++;
    } 
    
    if (pathname == NULL) {	 
        // 仅输入ls或ls -l, 无路径, 默认以当前路径的绝对路径。
        if (NULL != getcwd(final_path, MAX_PATH_LEN)) {
            pathname = final_path;
        } else {
            printf("ls: getcwd for default path failed.\n");
            return;
        }
    } else {
        // 有路径参数
        make_clear_abs_path(pathname, final_path);
        pathname = final_path;
    }

    if (stat(pathname, &file_stat) == -1) {
        printf("ls: cannot access %s: No such file or directory\n", pathname);
        return;
    }
    if (file_stat.st_filetype == FT_DIRECTORY) {
        struct dir* dir = opendir(pathname);
        struct dir_entry* dir_e = NULL;
        char sub_pathname[MAX_PATH_LEN] = {0};
        uint32_t pathname_len = strlen(pathname);
        uint32_t last_char_idx = pathname_len - 1;
        memcpy(sub_pathname, pathname, pathname_len);
        if (sub_pathname[last_char_idx] != '/') {
            sub_pathname[pathname_len] = '/';
            pathname_len++;
        }
        rewinddir(dir);
        if (long_info) {
            char ftype;
            printf("total: %d\n", file_stat.st_size);
            while((dir_e = readdir(dir))) {
                ftype = 'd';
                if (dir_e->f_type == FT_REGULAR) {
                    ftype = '-';
                } 
                sub_pathname[pathname_len] = 0;
                strcat(sub_pathname, dir_e->filename);
                memset(&file_stat, 0, sizeof(struct stat));
                if (stat(sub_pathname, &file_stat) == -1) {
                    printf("ls: cannot access %s: No such file or directory\n", dir_e->filename);
                    return;
                }
                printf("%c  %d  %d  %s\n", ftype, dir_e->i_no, file_stat.st_size, dir_e->filename);
            }
        } else {
        while((dir_e = readdir(dir))) {
            printf("%s ", dir_e->filename);
        }
        printf("\n");
        }
        closedir(dir);
    } else {
        if (long_info) {
            printf("-  %d  %d  %s\n", file_stat.st_ino, file_stat.st_size, pathname);
        } else {
            printf("%s\n", pathname);  
        }
    }
}

void buildin_ps(uint32_t argc, char** argv UNUSED) {
    if (argc != 1) {
        printf("ps: no argument support!\n");
        return;
    }
    ps();
}

void buildin_clear(uint32_t argc, char** argv UNUSED) {
    if (argc != 1) {
        printf("clear: no argument support!\n");
        return;
    }
    clear();
}

/* 有且仅有一个参数 */
int32_t buildin_mkdir(uint32_t argc, char** argv) {
    int32_t ret = -1;
    if (argc != 2) {
        printf("mkdir: only support 1 argument!.\n");
    } else {
        make_clear_abs_path(argv[1], final_path);
        /* 若创建的不是根目录 */
        if (strcmp("/", final_path)) {
            if (mkdir(final_path) == 0) {  // 成功返回0，失败-1
                ret = 0;
            } else {
                printf("mkdir: create directory %s failed.\n", argv[1]);
            }
        }
    }
    return ret;
}

/* 有且仅有一个参数 */
int32_t buildin_rmdir(uint32_t argc, char** argv) {
    int32_t ret = -1;
    if (argc != 2) {
        printf("rmdir: only support 1 argument!.\n");
    } else {
        make_clear_abs_path(argv[1], final_path);
        /* 若删除的不是根目录 */
        if (strcmp("/", final_path)) {
            if (rmdir(final_path) == 0) {
                ret = 0;
            } else {
                printf("rmdir: remove %s failed.\n", argv[1]);
            }
        }
    }
    return ret;
}

/* 有且仅有一个参数, 删除文件函数 */
int32_t buildin_rm(uint32_t argc, char** argv) {
    int32_t ret = -1;
    if (argc != 2) {
        printf("rm: only support 1 argument!.\n");
    } else {
        make_clear_abs_path(argv[1], final_path);
        if (strcmp("/", final_path)) {
            if (unlink(final_path) == 0) {
                ret = 0;
            } else {
                printf("rm: delete %s failed.\n", argv[1]);
            }
        }
    }
    return ret;
}
