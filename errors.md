219页倒数第二行e_entry写错了，0xc0101500->0xc0001500
244页第十段数值大小书写错误
#代码：
`int main(void){
    while(1);
    return 0;
}`
文件的内容:
`./src/tools/xxd.sh ./bin2/kernel.bin 0 300`
`00000000: 7F 45 4C 46 01 01 01 00 00 00 00 00 00 00 00 00  .ELF............
00000010: 02 00 03 00 01 00 00 00 00 15 00 C0 34 00 00 00  ............4...
00000020: 00 06 00 00 00 00 00 00 34 00 20 00 03 00 28 00  ........4. ...(.
00000030: 06 00 05 00 01 00 00 00 00 00 00 00 00 10 00 C0  ................
00000040: 00 00 00 C0 94 00 00 00 94 00 00 00 04 00 00 00  ................
00000050: 00 10 00 00 01 00 00 00 00 05 00 00 00 15 00 C0  ................
00000060: 00 15 00 C0 05 00 00 00 05 00 00 00 05 00 00 00  ................
00000070: 00 10 00 00 51 E5 74 64 00 00 00 00 00 00 00 00  ....Q.td........
00000080: 00 00 00 00 00 00 00 00 00 00 00 00 06 00 00 00  ................
00000090: 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
000000a0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
*
00000120: 00 00 00 00 00 00 00 00 00 00 00 00 `

分析:
  e_entry:00 15 00 C0->程序的虚拟入口地址0xc0001500
  e_pjoff:34 00 00 00->程序头表在文件中的偏移量:0x34
  e_shoff:00 06 00 00->节头表在文件内偏移量：0x600
  第三行的e_ehsize:34 00->elf header大小:0x34,与e_phoff一致
  e_phentsize:20 00->program header的结构(struct Elf32_Phdr的字节大小):0x20
  e_phnum:03 00,程序头表中段的个数:0x03
  e_shentsize:28 00,节头表中各个节的大小:0x0028
  e_shstrndx:06 00,节头表中节的个数
  第6到8行可以看出可执行段的：
  本段在文件内偏移量：00 05 00 00->0x00000500
  本段被加载到内存后的起始虚拟地址：00 15 00 C0->0xc0001500
  p_filesz:本段在文件中的字节大小,05 00 00 00->0x05

查看:
  `./src/tools/xxd.sh ./bin2/kernel.bin 0x500 10`
  输出: 00000500: 55 89 E5 EB FE 47 43 43 3A 20


perfect!··