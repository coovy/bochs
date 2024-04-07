;----------详看p433
[bits 32]
section .text
global switch_to
switch_to:          ;两个参数:当前线程pcb和下一个线程pcb
    ;栈中此时是返回地址
    push esi
    push edi
    push ebx
    push ebp    ;abi规定这四个寄存器+esp归主调函数所有，其余归被调函数

    mov eax, [esp+20]   ; // 得到栈中的参数cur, cur = [esp + 20]， 四个寄存器+返回地址
    mov [eax], esp  ;保存栈指针esp, task_struct的self_kstack字段
                    ;self_kstack在task_struct中的偏移为0
                    ;所以直接往thread开头处存4字节便可

;-----备份完当前线程环境，开始恢复下一个线程的环境--------

    mov eax, [esp+24]   ;得到栈中的参数next, next = [esp+24]
    mov esp, [eax]      ;pcb的第一个成员是self_kstack成员
                        ;用来记录0级栈顶指针，被换上cpu时用来恢复0级栈
                        ;0级栈中保存了进程或者线程的所有信息，包括3级栈指针
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret     ;返回上面switch_to下面那句注释的返回地址
            ;未由中断进入，第一次执行时会返回到kernel_thread