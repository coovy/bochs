

char *str = "hello,world\n";
int count = 0;

void main(void)
{
    asm("pusha;\
        movl $4, %eax;\             //功能号，
        movl $1, %ebx;\             //下面传参,标准输出的文件描述符是1
        movl str, %ecx;\
        movl $12, %edx;\
        int $0x80;\
        mov %eax,count;\
        popa");
}