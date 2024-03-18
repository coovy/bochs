%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR  ;0x900
LOADER_STACK_TOP equ LOADER_BASE_ADDR   ;0x900

;构建gdt及其内部的描述符
GDT_BASE: dd 0x0000_0000 
            dd 0x0000_0000
CODE_DESC: dd 0x0000_FFFF 
            dd DESC_CODE_HIGH4
DATA_STACK_DESC: dd 0x0000_FFFF
                    dd DESC_DATA_HIGH4

VIDEO_DESC: dd 0x8000_0007         		   ;这里不采用平坦模型,段界限limit=(0xbffff-0xb8000)/4k=0x7
                dd DESC_VIDEO_HIGH4

GDT_SIZE equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE - 1
    
    times 60 dq 0       ;此处预留60个描述符的空位     

;total_mem_bytes用于保存内存容量,以字节为单位，此位置比较好记
;当前偏移loader.bin文件头0x200字节(4*8+60*8=512B)
;loader.bin加载地址是0x900
;故total_mem_bytes内存中的地址是0xb00
;将来在内核中咱们会引用此地址
total_mem_bytes dd 0

;选择子 15-3:索引  2:TI  1-0：RPL
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0  ;0001000b
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0  ;...11000=18h
;gdt指针,前2字节是gdt界限 后4字节gdt起始地址 
gdt_ptr: dw GDT_LIMIT
            dd GDT_BASE

    ;人工对齐: total_mem_bytes+gdt_ptr+ards_buf+ards_nr=4+6+244+2=256字节
    ;使得;loader_start地址0x300，属于作者乐趣
    ards_buf times 244 db 0
    ards_nr dw 0        ;用于记录ARDS结构体数量,nr->number

;loader_start文件内偏移地址0x300
loader_start:
;int 15h eax=0000E820h, edx=534D4150h('SMAP')固定签名获取内存布局
    mov sp,LOADER_BASE_ADDR
    xor ebx,ebx         ;第一次调用时，ebx值要为0
    mov edx,0x534D4150
    mov di,ards_buf     ;ards结构缓冲区
.e820_mem_get_loop:
    mov eax, 0x0000e820 ;执行int 0xl5后,eax值变为Ox534d4150,所以每次执行前要更新为子功能号
    mov ecx, 20
    int 0x15
    jc .e820_failed_so_try_e801 ;若cf为1则有错误
    add di,cx           ;使di增加20字节指向缓冲区中新的ARDS结构位置
    inc word [ards_nr]
    cmp ebx,0           ;若ebx为0且cf不为1，这说明ards全部返回
    jnz .e820_mem_get_loop
    ;在所有ards结构中找出(base_add_low + length_low)最大值，即内存的容量
    mov cx, [ards_nr]
    ;遍历所有ards结构体
    mov ebx, ards_buf
    xor edx,edx
.find_max_mem_area:
;无需判断type是否为1，最大的内存块一定是可被使用的
    mov eax,[ebx]
    add eax,[ebx+8]
    add ebx,20      ;指向缓冲区下一个ards结构
    cmp edx,eax
    ;冒泡排序，找出最大，edx寄存器始终是最大的内存容量
    jge .next_ards
    mov edx,eax         ;edx为总内存大小
.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok
.e820_failed_so_try_e801:
   mov ax,0xe801
   int 0x15
   jc .e801_failed_so_try88   ;若当前e801方法失败,就尝试0x88方法

;1 先算出低15M的内存,ax和cx中是以KB为单位的内存数量,将其转换为以byte为单位
   mov cx,0x400	     ;cx和ax值一样,cx用做乘数
   mul cx 
   shl edx,16
   and eax,0x0000FFFF
   or edx,eax
   add edx, 0x100000 ;ax只是15MB,故要加1MB
   mov esi,edx	     ;先把低15MB的内存容量存入esi寄存器备份

;2 再将16MB以上的内存转换为byte为单位,寄存器bx和dx中是以64KB为单位的内存数量
   xor eax,eax
   mov ax,bx		
   mov ecx, 0x10000	;0x10000十进制为64KB
   mul ecx		;32位乘法,默认的被乘数是eax,积为64位,高32位存入edx,低32位存入eax.
   add esi,eax		;由于此方法只能测出4G以内的内存,故32位eax足够了,edx肯定为0,只加eax便可
   mov edx,esi		;edx为总内存大小
   jmp .mem_get_ok

;-----------------  int 15h ah = 0x88 获取内存大小,只能获取64M之内  ----------
.e801_failed_so_try88: 
   ;int 15后，ax存入的是以kb为单位的内存容量
   mov  ah, 0x88
   int  0x15
   jc .error_hlt
   and eax,0x0000FFFF
      
   ;16位乘法，被乘数是ax,积为32位.积的高16位在dx中，积的低16位在ax中
   mov cx, 0x400     ;0x400等于1024,将ax中的内存容量换为以byte为单位
   mul cx
   shl edx, 16	     ;把dx移到高16位
   or edx, eax	     ;把积的低16位组合到edx,为32位的积
   add edx,0x100000  ;0x88子功能只会返回1MB以上的内存,故实际内存大小要加上1MB
; --------------------------------- 设置进入保护模式 -----------------------------
; 1 打开A20 gate
; 2 加载gdt
; 3 将cr0的pe位置1
    
    in al,0x92
    or al,0000_0010b
    out 0x92,al                ;打开A20 gate
   
    lgdt [gdt_ptr] 
    
    mov eax,cr0                ;cr0寄存器第0位设置位1
    or  eax,0x00000001              
    mov cr0,eax
      
;已打开保护模式
    jmp dword SELECTOR_CODE:p_mode_start ;刷新流水线(实模式16位指令，防止将后面32位指令译成16位，采用无条件跳转)

.error_hlt:		      ;出错则挂起
   hlt

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ;---------加载kernel-------
    mov eax, KERNEL_START_SECTOR    ;kernel.bin扇区号
    mov ebx, KERNEL_BIN_BASE_ADDR
    ;从磁盘读出后，写入到ebx指定的位置
    mov ecx, 200    ;读入的扇区数

    call rd_disk_m_32


    ;创建页目录及页表并初始化页内存位图
    call setup_page
    ;要将描述符表地址及偏移量写入内存gdt_ptr,一会儿用新地址加载
    ; sgdt [gdt_ptr]      ;存储到原来gdt所有的位置
    ;将gdt描述符中视频段描述符中的段基址+0xc0000000
    mov ebx,[gdt_ptr+2]     ;取高4字节的基址
    or dword [ebx+0x18+4],0xc0000000

    add dword [gdt_ptr+2],0xc0000000
    add esp,0xc0000000

    ;把页目录地址赋给cr3
    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax             ;cr3寄存器用来存储页目录物理地址,如果重新加载此寄存器会使得TLB失效

    ;打开cr0的pg位(第31位)
    mov eax,cr0
    or eax,0x80000000
    mov cr0,eax
    ;在开启分页后，用gdt新的地址更新

    lgdt [gdt_ptr]             ; 重新加载
    mov byte [gs:160],'V'

    jmp SELECTOR_CODE:enter_kernel      ;强制刷新流水线，更新gdt
enter_kernel:
    call kernel_init
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT
    ;jmp $
0x9fc0
;---------------将kernel.bin中的segment拷贝到编译的地址-----------
kernel_init:
    xor eax,eax
    xor ebx,ebx     ;ebx记录程序头表地址
    xor ecx,ecx     ;cx记录程序头表中的program header数量
    xor edx,edx     ;dx记录program header尺寸，即e_phentsize

    mov dx, [KERNEL_BIN_BASE_ADDR+42]   ;偏移42字节处的属性是e_phentsize, 表示program header大小
                                        ;偏移文件开始部分2 8 字节的地方是e_phoff
                                        ;表示第1个program header在文件中的偏移量
                                        ;其实该值是0x34,不过还是谨慎一点，这里来读取实际值

    add ebx, KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR+44]   ;偏移文件开始部分44字节的地方是e_phnum,表示有几个program header

.each_segment:
    cmp byte [ebx+0], PT_NULL   ;若p_type等于PT_NULL,说明此program header未使用
    je .PTNULL                  ;为函数memcpy压入参数，参数是从右往左依然压入
                                ;函数原型类似于memcpy(dst, src, size)
    push dword [ebx+16]         ;program header中偏移16字节的地方是p_filesz
                                ;压入函数memcpy 的第三个参数：size

    mov eax, [ebx+4]            ;距程序头偏移量为4字节的位置是p_offset,这个是本段对于文件头的偏移
    add eax, KERNEL_BIN_BASE_ADDR   ;加上kernel.bin被加载到的物理地址，eax为该段的物理地址

    push eax                    ;压入函数memcpy的第二个参数，源地址
    push dword [ebx+8]          ;压入函数memcpy的第一个参数，目的地址
                                ;偏移程序头8字节的位置是p_vaddr, 这就是目的地址

    call mem_cpy                ;调用mem_cpy完成段复制
    add esp,12
.PTNULL:
    add ebx, edx

    loop .each_segment
    ret

;------逐字节拷贝mem_cpy(dst,src,size)------
;输入：栈中三个参数(dst,src,size)
;输出：无
;---------------------
mem_cpy:
    cld
    push ebp
    mov ebp,esp
    push ecx     ;rep指令用到了ecx

    mov edi,[ebp+8]        ;dst
    mov esi,[ebp+12]       ;src
    mov ecx,[ebp+16]       ;size
    rep movsb              ;逐字节拷贝,;movsb指令相当于做如下操作：
                           ;((es)*16 + (edi)) = ((ds)*16 + (esi)) 

    ;恢复环境
    pop ecx
    pop ebp
    ret


;--------创建页目录及页表--------
setup_page:
;先把页目录占用的空间逐字节清0
    mov ecx,4096      ;4096=4KB空间
    mov esi,0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS+esi],0
    inc esi
    loop .clear_page_dir
;开始创建页目录项
.create_pde:
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000      ;加4KB,此时eax刚好为第一个页表的位置
    mov ebx,eax         ;此处为ebx赋值,是为.create_pte做准备,ebx为基址
;下面将页目录项0和0xc00都存为第一个页表的地址,每个页表表示4MB内存
;这样0xc03fffff以下的地址和0x003ffff 以下的地址都指向相同的页表
;这是为将地址映射为内核地址做准备
    or eax,PG_US_U|PG_RW_W|PG_P     ;页目录项的属性RW和p位为1,us为1,表示用户属性，所有特权级别都可以访问
    mov [PAGE_DIR_TABLE_POS + 0x0],eax  ;第一个目录项
                                        ;在页目录表中的第1个目录项写入第一个页表位置(0x101000)及属性
    mov [PAGE_DIR_TABLE_POS + 0xc00],eax    ;0xc00以上目录项用于内核，0xc00表示第768个页表占用的目录项
                                            ;即页表的0xc0000000~0xffffffff共计lG属于内核
    sub eax,0x1000
    mov [PAGE_DIR_TABLE_POS+4092],eax       ;使最后一个目录项指向页目录表自己的位置

;下面创建页表项(PTE)
    mov ecx,256             ;1MB低端内存/每页大小4K=256
    mov esi,0
    mov edx,PG_US_U | PG_RW_W | PG_P    ;属性为111b

.create_pte:
    mov [ebx+esi*4],edx
    add edx,0x1000
    inc esi
    loop .create_pte

;创建内核其他页表的PDE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000              ;此时eax为第二个页表的位置
    or eax,PG_US_U | PG_RW_W | PG_P
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254                 ;范围为第769-1022 的所有目录项数量
    mov esi,769
.create_kernel_pde:
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde
    ret