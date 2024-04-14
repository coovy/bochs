TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

[bits 32]
section .data
put_int_buffer dq 0     ;定义8字节缓冲区用于数字到字符的转换



section .text
;------------------------
;put_str 通过put_char来打印以0字符结尾的字符串
;------------------------
;输出:栈中参数为打印的字符串
;输出:无
global put_str
put_str:
    ;由于本函数中只用到了ebx和ecx，只备份这两个寄存器
    push ebx
    push ecx
    xor ecx,ecx
    mov ebx,[esp+12]

.goon:
    mov cl,[ebx]
    cmp cl,0
    jz .str_over
    push ecx
    call put_char
    add esp,4
    inc ebx
    jmp .goon

.str_over:
    pop ecx
    pop ebx
    ret

;----功能描述: 把栈中的1个字符写到光标所在处----------
global put_char
put_char:
    pushad      ;备份32位寄存器环境
    ;需要保gs中为正确的视频段选择子
    ;为保险起见，每次打印时都为gs赋值
    mov ax,SELECTOR_VIDEO   ;不能直接把立即数送入段寄存器
    mov gs,ax

;--------获取当前光标位置-------
    ;先获得高八位
    ;先写入索引号，再写入数据寄存器端口号，再读取高8位数据
    mov dx,0x03d4       ;索引寄存器
    mov al,0x0e         ;用于提供光标位置的高8位,0EH：Cursor Location High Register
    out dx,al
    mov dx,0x03d5       ;通过读写数据端口0x3d5来获得或设置光标位置
    in al,dx            ;得到了光标位置的高8位
    mov ah,al

    ;再获取低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    in al,dx

    ;将光标存入bx
    mov bx,ax
    ;下面这行是在栈中获取待打印的字符
    mov ecx,[esp+36]        ;pushed压入4*8=32字节
                            ;加上主调函数4字节的返回地址，故esp+36字节
    cmp cl,0xd             ;CR(Carriage Return回车键)是0x0d,
    jz .is_carriage_return
    cmp cl,0xa              ;LF是0x0a
    jz .is_line_feed

    cmp cl,0x8              ;BS(backspace)的ascii码是8
    jz .is_backspace
    jmp .put_other
    
.is_backspace:
    dec bx              ;dec减一指令
    shl bx,1            ;光标乘2
                        ;表示光标对应显存中的偏移字节
    mov byte [gs:bx],0x20   ;将待删除的字节补为0或空格皆可,这里是空格
    inc bx
    mov byte [gs:bx],0x07   ;字符属性
    shr bx,1
    jmp .set_cursor

.put_other:
    shl bx,1            ;光标位置用2字节表示，将光标值乘2,
                        ;在默认的文本80*25中，光标值就算0~1999。
                        ;每个字符占2字节，低字节ascii，高字节属性
                        ;表示对应显存中的偏移字节
    mov [gs:bx],cl      ;ASCII字符本身
    inc bx
    mov byte [gs:bx],0x07   ;字符属性
    shr bx,1                ;恢复老的光标值
    inc bx                  ;下一个光标值
    cmp bx, 2000
    jl .set_cursor          ;若光标值小于2000,表示未写到
                            ;显存的最后，则去设置新的光标值
                            ;若超出屏幕字符数大小(2000)
                            ;则换行处理
.is_line_feed:               ;是换行符LF(\n)
.is_carriage_return:         ;是回车符CR(\r),移动光标到行首
    xor dx,dx                ;dx是被除数的高16位，清0
    mov ax,bx                ;ax是被除数的低16位
    mov si,80                ;把\n,\r都处理为Linux中\n的意思
    div si
    sub bx,dx                ;光标值减去80的余数便是取整
                             ;以上4行处理\r的代码

.is_carriage_return_end:
    add bx,80
    cmp bx,2000
.is_line_feed_end:          ;若是LF(\n),将光标移+80便可
    jl .set_cursor


;屏幕行范围是0~24，所以滚屏将1~25行,搬运到0~23行，再将第24行用空格填充
.roll_screen:               ;若超出屏幕大小，开始滚屏
    cld                     ;使标识寄存器的df为0，决定地址增大
    mov ecx,960             ;2000-80=1920个字符要搬运,共计1920*2=3840字节
                            ;一次搬4字节，3840/4=960次
    mov esi,0xc00b80a0      ;第一行行首
    mov edi,0xc00b8000      ;第0行行首
    rep movsd

;;;;;;;;将最后一行填充为空白
    mov ebx,3840            ;最后一行字符的第一个字节偏移=1920*2
    mov ecx,80

.cls:
    mov word [gs:ebx],0x0720    ;0x0720是黑底白字的空格键
    add ebx,2
    loop .cls
    mov bx,1920                 ;将光标重置为1920，最后一行的首字符

.set_cursor:
    ;将光标设为bx的值
    ;先设置高8位
    mov dx,0x03d4               ;索引寄存器
    mov al,0x0e                 ;用于提供光标位置的高8位
    out dx,al                   
    mov dx,0x03d5               ;通过读写数据端口0x03d5来获得或设置光标位置
    mov al,bh
    out dx,al
    ;再设置低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl
    out dx,al
    
.pu_char_done:
    popad
    ret

;将小端字节序的数字变成对应的ASCII 后，倒置
;输入：栈中参数为待打印的数字
;输出：在屏幕上打印十六进制数字，并不会打印前缀0x
;如打印十进制15，只会直接打印f，不会是0xf
global put_int
put_int:
    pushad
    mov ebp,esp
    mov eax,[ebp+36]    ;小端序是低位在低地址，高位在高地址,
                        ;从低地址开始取，取的是数值本身低位
    mov edx,eax
    mov edi,7           ;指定在put_buffer中初始的偏移量
    mov ecx,8           ;32位数字中，十六进制数字的位数是8个
    mov ebx,put_int_buffer

;将32位数字按照十六进制的形式从低位到高位逐个处理
;共处理8个十六位数字
.16based_4bits:          ;每4位二进制是十六进制数字的1位
;遍历每一位十六进制数字
    and edx,0x0000000F   ;解析十六进制数字的每一位
                         ;and与操作后，edx只有低4位有效
    cmp edx,9
    jg .is_A2F
    add edx,'0'          ;ascii码是8位，add求和后，edx低8位有效
    jmp .store

.is_A2F:
    sub edx,10           ;A~F减去10所得的差，
                         ;再加上字符A的ascii码，
                         ;便是A~F对应的ascii码
    add edx,'A'
    ;将每一位数字转换成对应的字符后，按照类似“大端”的顺序
    ;存储待缓冲区put_int_buffer
    ;高位字符放到低地址，低位字符要放到高地址，这样和大端字节序类似，
    ;只不过这里是字符序

.store:
    ;此时dl中应该是数字对应的ascii码
    mov [ebx+edi],dl    ;edi是从7开始,共处理8次,
                        ;就把原先处于低位字节的低位数据，放在高地址
    dec edi
    shr eax,4
    mov edx,eax
    loop .16based_4bits  ;loop操作修改cx值并判断

;现在put_int_buffer中已全是字符,打印之前
;把高位连续的字符去掉，比如把字符000123变成123
.ready_to_print:
    inc edi             ;此时edi退减为-1(0xffffffff),加1使其为0
.skip_prefix_0:
    cmp edi,8           ;若已经比较第9个字符了
                        ;表示待打印的字符串为全0
    je .full0               

;找出连续的0字符,edi作为非0的最高位字符的偏移
.go_on_skip:
    mov cl,[put_int_buffer+edi]     ;从低字节(此时是数据高位)开始取
    inc edi
    cmp cl,'0'
    je .skip_prefix_0   ;继续判断下一位字符是否为字符0(不是数字0)
    dec edi             ;edi在上面的inc操作中指向了下一个字符
                        ;若当前字符不为'0',要使edi减1恢复指向当前字符
    jmp .put_each_num

.full0:
    mov cl,'0'          ;输入的数字为全0时，则只打印0

.put_each_num:
    push ecx            ;此时cl中为可打印字符
    call put_char
    add esp,4
    inc edi             ;使edi指向下一个字符
    mov cl, [put_int_buffer+edi]
    cmp edi,8
    jl .put_each_num
    popad
    ret

global set_cursor
set_cursor:
   pushad
   mov bx, [esp+36]
;1 先设置高8位
   mov dx, 0x03d4		;索引寄存器
   mov al, 0x0e			;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5		;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al

;2 再设置低8位
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret