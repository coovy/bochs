LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2 ;起始扇区lba地址
LOADER_STACK_TOP equ LOADER_BASE_ADDR

PAGE_DIR_TABLE_POS equ 0x100000

KERNEL_START_SECTOR equ 9
KERNEL_ENTRY_POINT equ 0xc0001500
KERNEL_BIN_BASE_ADDR equ 0x70000

;-----gdt描述符属性-----
DESC_G_4K equ       1_00000000000000000000000b  ;段描述符的G(23)位:0-1B,1-4KB, 平坦模型段界限边界值就是2*20 * 4KB = 4GB
DESC_D_32 equ        1_0000000000000000000000b  ;D/B(22)位, 32位地址和操作数
DESC_L    equ         0_000000000000000000000b  ;L(21)位， 0-非64位编码
DESC_AVL  equ          0_00000000000000000000b  ;avl(available)(20),对用户来说的“可用位”，操作系统无限制
DESC_LIMIT_CODE2  equ   1111_0000000000000000b  ;16-19位:段界限后四位
DESC_LIMIT_DATA2  equ  DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2 equ   0000_0000000000000000b  

DESC_P  equ    1_000000000000000b    ;P(Present)(15)字段, 1-段存在于内存中，0-不存在，报异常
DESC_DPL_0 equ  00_0000000000000b    ;DPL(Descriptor Privilege Level)(13-14)三个特权级
DESC_DPL_1 equ  01_0000000000000b
DESC_DPL_2 equ  10_0000000000000b
DESC_DPL_3 equ  11_0000000000000b

DESC_S_CODE       equ  1_000000000000b           ;第12位S与type字段配合
DESC_S_DATA       equ  DESC_S_CODE
DESC_S_sys        equ  0_000000000000b           ;第12位为0-系统段，1-数据段

DESC_TYPE_CODE    equ   1000_00000000b            ;8-11位type段(xewa) 1000==可执行，非一致性，不允许可读，已访问位0, 注意书上图标记顺序和xewa不一致
DESC_TYPE_DATA    equ   0010_00000000b            ;8-11位type段   0010==不可执行，向上扩展，可写，已访问位0 

; 代码段高4字节
DESC_CODE_HIGH4   equ  (0x00<<24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + \
DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0X00

DESC_DATA_HIGH4   equ  (0x00<<24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + \
DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0X00

;实模式下显存中用于显示文本模式的起始地址为0xB8000, 
;loader.S中的低16为段基址是0x8000，所以这里应该+0x0B
DESC_VIDEO_HIGH4   equ (0x00<<24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + \
DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0X0B

RPL0    equ 00b
RPL1    equ 01b
RPL2    equ 10b
RPL3    equ 11b
TI_GDT  equ 000b   ; TI=0->GDT, rpl=00
TI_LDT  equ 100b

;-----页表相关属性-----
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b

;-------------  program type 定义   --------------
PT_NULL equ 0