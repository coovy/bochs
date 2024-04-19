#ifndef __DIR_H
#define __DIR_H
#define MAX_FILE_NAME_LEN 16

#include "stdint.h"

/* 目录结构, 并不会在磁盘存储 */
struct dir {
    struct inode *inode;
    uint32_t dir_pos;   // 记录在目录内的偏移，遍历目录时使用
    uint8_t dir_buf[512];   // 目录的数据缓存
};

/* 目录项结构 */
struct dir_entry {
    char filename[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
    uint32_t i_no;  // 普通文件或目录对应的inode编号
    enum file_types f_type; // 文件类型
};

#endif