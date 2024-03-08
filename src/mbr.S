%include "boot.inc"

SECTION MBR vstart=0x7c00 ;设定mbr的起始地址
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00

    mov ax,0xb800   ;现存在文本模式， 内存地址是0xb800
    mov gs,ax

    mov ax,0x0600        ;AH: 功能号0x06, 上卷窗口, AL:上卷行数，0-表示全部
    mov bx,0x0700        ;BH: 上卷行属性
    mov cx,0            ;(CL,CH)=窗口左上角(X,Y)位置
    mov dx,0x184f       ;;(DL,DH)=窗口右下角(X,Y)位置, (80,25)

    int 0x10            ;执行中断

    ; ;下面三行获取光标位置
    ; mov ah,3            ;3号子功能获取光标位置
    ; mov bh,0            ;bh寄存器存储的是待获取光标的页号
    
    ; int 0x10            ;输出：ch=光标开始行，cl=光标结束行
    ;                     ;dh=光标所在行号, dl=光标所在列号

    ;打印字符串
    ; mov ax,message
    ; mov bp,ax
    
    ; mov cx,5            ;cx:字符串长度, 不包括结束符0的字符个数
    ; mov ax,0x1301       ;13号子功能打印字符串， al: 写字符方式，ah=01:显示字符串，光标跟随一堆

    ; mov bx,0x2          ;bh:显示的页号，0页，bl: 字符属性，黑底绿字(bl=02h)
    
    ; int 0x10            ;再次执行中断
    
    ;操作显存打印字符串
    mov byte [gs:0x00], '1'
    mov byte [gs:0x01], 0xA4 ;A表示绿色背景闪烁, 4表示前景色为红色

    mov byte [gs:0x02], ' '
    mov byte [gs:0x03], 0xA4

    mov byte [gs:0x04], 'M'
    mov byte [gs:0x05], 0xA4

    mov byte [gs:0x06], 'B'
    mov byte [gs:0x07], 0xA4

    mov byte [gs:0x08], 'R'
    mov byte [gs:0x09], 0xA4

    mov eax, LOADER_START_SECTOR    ;起始扇区lba地址
    mov bx, LOADER_BASE_ADDR     ;写入的地址
    mov cx, 1                       ;待读入的扇区
    call rd_disk_m_16               ;以下读取程序的起始部分(一个扇区)
    
    jmp LOADER_BASE_ADDR

    ;读取硬盘n个扇区
rd_disk_m_16:
                    ;eax=LBA扇区号
                    ;bx=将数据写入的内存地址
                    ;cx=读入的扇区数
    mov esi,eax     ;备份
    mov di,cx       ;备份

    ;设置读取扇区数
    mov dx,0x1f2    ;Sector count:读取或者写入扇区数
    mov al,cl
    out dx,al       ;读取扇区数, al=0000 0001

    mov eax,esi     ;恢复ax

    ;将LBA地址存入0x1f3~0x1f6
    ;7-0位写入0x1f3
    mov dx,0x1f3
    out dx,al

    ;LBA地址15-8位写入端口0x1f4
    mov cl,8
    shr eax,cl  ;eax原本是起始扇区地址
    mov dx,0x1f4
    out dx,al

    ;LBA地址23-16位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f     ;LBA地址24-27位
    or al,0xe0      ;设置7-4位为1110，表示lba模式。

    mov dx,0x1f6
    out dx,al

    ;向0x1f7端口写入读命令，0x20
    mov dx,0x1f7
    mov al,0x20       
    out dx,al       ;0x1f7: 0010 0000

    ;检测硬盘状态,检测status寄存器的BSY位------
  .not_ready:
    ;同一端口，写时表示写入命令字，读时表示读入硬盘状态
    nop
    in al,dx
    and al,0x08
    cmp al,0x08
    jnz .not_ready
    ;-----

    ;从0x1f0端口读取数据
    mov ax,di
    mov dx,256
    mul dx      ;另一个乘数在ax
    mov cx,ax

    ;di为要读取的扇区数,一个扇区有512字节,每次读入一个字
    ;共需di*512/2次，所以di*256
    mov dx,0x1f0    ;0x1f0: data寄存器
  .go_on_read:
    in ax,dx
    mov [bx],ax
    add bx,2        ;如果读取数据超过2^16=64KB会回绕，覆盖数据
    loop .go_on_read
    ret

    ;jmp $               ;程序悬停
    
    ; message db "1 MBR" 
    
    times 510 - ($ - $$) db 0 
    db 0x55,0xaa
