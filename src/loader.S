%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR  ;0x900
LOADER_STACK_TOP equ LOADER_BASE_ADDR   ;0x900
   
    jmp loader_start
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
    
    times 60 dq 0                             	   ;此处预留60个描述符的空位
;选择子 15-3:索引  2:TI  1-0：RPL
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0  ;0001000b
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0
    
;gdt指针,前2字节是gdt界限 后4字节gdt起始地址 
gdt_ptr: dw GDT_LIMIT
            dd GDT_BASE
    
loadermsg db '2 loader in real.'
;INT 0x10 功能号：0x13 功能描述: 打印字符串
;输入：
;AH子功能号＝13H
;BH＝页码
;BL＝属性（若AL=00H或者01H)
;CX＝字符串长度
;(DH,DL)＝坐标(行、列)
;ES:BP＝字符串地址
;AL＝显示输出方式
;   0--字符串中只含显示字符，其显示属性在BL
;       显示后，光标位置不变
;   1--字符串中只含显示字符，其显示属性在BL
;       显示后，光标位置改变
;   2--字符事中含显示字符和显示属性。显示后，光标位置不变
;   3--字符串中含显示字符和显示属性。显示后，光标位置改变
;无返回值
loader_start:            
;jmp loader_start3字节机器码+4个段描述符+预留的60描述符大小+gdt_ptr的6字节+lodermsg的17字节
;=3+32+480+6+17=538=0x21a, loader起始地址是0x900, 0x21a+0x900=0xb1a,则loader_start起始地址为0xb1a-4=0xb17
    mov sp,LOADER_BASE_ADDR
    mov bp,loadermsg    ;es:bp=字符串地址
    mov cx,17           ;字符串长度                     
    mov ax,0x1301       ;AH=13H,AL=01H
    mov bx,0x001f       ;页号为0(BH=0) 蓝底粉红字(BL=1fh)
    mov dx,0x1800       ;dh=24 最后一行,0列开始
    int 0x10
    
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
 
[bits 32]
p_mode_start: 
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax

    mov byte [gs:160],'P'   ;每行80个字符,160字节，所以这里是第二行第一个位置
    jmp $       


