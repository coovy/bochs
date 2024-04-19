#ifndef __INODE_H
#define __INODE_H
#include "stdint.h"
#include "list.h"

/* inode结构 */
struct inode {
    uint32_t i_no;  // inode编号

    /* 当此inode是文件时，i_size是文件大小
    如果是目录，指的是该目录下所有目录项大小之和 */
    uint32_t i_size;

    uint32_t i_open_cnts;   // 记录此文件被打开的次数
    bool write_deny;    // 写文件不能并行，进程写文件前检查此标志

    /* i_sectors[0-11]是直接块，i_sectors[12]用来存储一级间接块指针 */
    /* 这里实现的块大小==扇区， 扇区512B/4B=128, 块地址是4B, 所以支持128个一级间接块， 共12+128=140个块 */
    uint32_t i_sectors[13];
    struct list_elem inode_tag;
};
#endif