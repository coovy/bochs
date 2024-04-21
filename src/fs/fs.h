#ifndef __FS_H
#define __FS_H

#include "stdint.h"

#define MAX_FILES_PER_PART 4096
// 每个分区支持的最大创建的文件数
#define BITS_PER_SECTOR 4096 // 每扇区的位数
#define SECTOR_SIZE 512 // 每扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE // 块字节大小

#define MAX_PATH_LEN 512        // 路径最大长度， 这个指的是整个路径字符串的长度

/* 文件类型 */
enum file_types
{
   FT_UNKNOWN,  // 不支持的文件类型
   FT_REGULAR,  // 普通文件
   FT_DIRECTORY // 目录
};

/* 打开文件的选项 */
enum oflags {
   O_RDONLY,      // 000b 只读
   O_WRONLY,      // 001b 只写
   O_RDWR,      // 010b 读写
   O_CREAT = 4      //100b 创建
};


/* 文件读写位置偏移量 */
enum whence
{
   SEEK_SET = 1,  // 新读写位置是相对于文件开头再增加offset个位移量 
   SEEK_CUR,      // 新读写位置是相对于当前的位置增加offset个位移量
   SEEK_END       // 新读写位置是相对于文件尺寸再增加offset个位移量, 此情况下,offset应该为负值
};

/* 用来记录查找文件过程中已找到的上级路径 */
struct path_search_record {
   char searched_path[MAX_PATH_LEN];        // 查找过程中的父路径
   struct dir* parent_dir;            // 文件或目录所在的直接父目录，无论是对文件和子目录的操作都会影响到直接父目录的内容
   enum file_types file_type;            // 找到的是普通文件还是目录, 找不到为未知类型(FT_UNKNOWN)
};

/* 文件属性结构体，Linux中ls命令就是通过stats获取文件属性的，stat64是64位版的stat */
struct stat {
   uint32_t st_ino;  // inode编号
   uint32_t st_size;    // 尺寸
   enum file_types st_filetype;  // 文件类型
};

extern struct partition *cur_part;
void filesys_init(void);
int32_t path_depth_cnt(char* pathname);
int32_t sys_open(const char* pathname, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);
int32_t sys_read(int32_t fd, void* buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char* pathname);
int32_t sys_mkdir(const char* pathname);
struct dir* sys_opendir(const char* pathname);
int32_t sys_closedir(struct dir* dir);
struct dir_entry* sys_readdir(struct dir* dir);
void sys_rewinddir(struct dir* dir);
int32_t sys_rmdir(const char *pathname);
#endif