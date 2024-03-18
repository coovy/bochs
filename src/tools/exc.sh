#!/bin/bash

# nasm -I /home/yhy/workspace/cccc/bochs/src/include -o /home/yhy/workspace/cccc/bochs/bin2/mbr.bin /home/yhy/workspace/cccc/bochs/src/boot/mbr.S
# dd if=/home/yhy/workspace/cccc/bochs/bin2/mbr.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=1 conv=notrunc
# nasm -I /home/yhy/workspace/cccc/bochs/src/include -o /home/yhy/workspace/cccc/bochs/bin2/loader.bin /home/yhy/workspace/cccc/bochs/src/boot/loader.S
# dd if=/home/yhy/workspace/cccc/bochs/bin2/loader.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=3 seek=2 conv=notrunc

# gcc -m32 -c -o /home/yhy/workspace/cccc/bochs/bin2/main.o /home/yhy/workspace/cccc/bochs/src/kernel/main.c && ld -m elf_i386 /home/yhy/workspace/cccc/bochs/bin2/main.o -Ttext 0xc0001500 -e main -o /home/yhy/workspace/cccc/bochs/bin2/kernel.bin
# dd if=/home/yhy/workspace/cccc/bochs/bin2/kernel.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=200 seek=9 conv=notrunc

/home/yhy/workspace/cccc/bochs/bin/bochs -f /home/yhy/workspace/cccc/bochs/src/bochsrc.disk