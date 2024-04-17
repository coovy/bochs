#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "sync.h"
#include "list.h"
#include "bitmap.h"

/* 分区的结构 */
struct partition {
   uint32_t start_lba;            // 分区起始扇区, 并不是绝对的起始扇区地址，因为还有前面不在分区内的部分
   uint32_t sec_cnt;               // 扇区数
   struct disk* my_disk;        // 分区所属的硬盘
   struct list_elem part_tag;     // 用于队列中的标记
   char name[8];                   // 分区名称
   struct super_block* sb;          // 本分区的超级块
   struct bitmap block_bitmap;   // 块位图, 为了减少对低速磁盘的访问次数，文件系统通常以多个扇区为单位来读写磁盘，这多个扇区就是块
   struct bitmap inode_bitmap;    // i结点位图
   struct list open_inodes;       // 本分区打开的i结点队列
};

/* 硬盘结构 */
struct disk {
   char name[8];               // 本硬盘的名称，如sda, sdb等
   struct ide_channel* my_channel;       // 此块硬盘归属于哪个ide通道,一个通道两个硬盘
   uint8_t dev_no;               // 本硬盘是主0还是从1
   struct partition prim_parts[4];       // 主分区顶多是4个
   struct partition logic_parts[8];       // 逻辑分区数量无限,但应有支持上限,这里支持8个
};

/* ata通道结构，也就是ide通道 */
struct ide_channel {
   char name[8];             // ata通道名称, 如ata0,也被叫做ide0. 可以参考bochs配置文件中关于硬盘的配置
   uint16_t port_base;         // 本通道的起始端口号
   uint8_t irq_no;           // 本通道所用的中断号
   struct lock lock;       // 每次读写硬盘时会申请锁,保证同步一致性

   /*表示等待硬盘的中断*/
   bool expecting_intr;         

   struct semaphore disk_done;   // 硬盘处理完成。线程用这个信号量来阻塞自己，由硬盘完成后产生的中断将线程唤醒
   struct disk devices[2];       // 一个通道上连接两个硬盘，一主一从
};

void intr_hd_handler(uint8_t irq_no);
void ide_init(void);
extern uint8_t channel_cnt;
extern struct ide_channel channels[];
extern struct list partition_list;
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
#endif
