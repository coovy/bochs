#!/bin/bash

# nasm -I /home/yhy/workspace/cccc/bochs/src/include -o /home/yhy/workspace/cccc/bochs/build/bin/mbr.bin /home/yhy/workspace/cccc/bochs/src/boot/mbr.S
# dd if=/home/yhy/workspace/cccc/bochs/build/bin/mbr.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=1 conv=notrunc
# nasm -I /home/yhy/workspace/cccc/bochs/src/include -o /home/yhy/workspace/cccc/bochs/build/bin/loader.bin /home/yhy/workspace/cccc/bochs/src/boot/loader.S
# dd if=/home/yhy/workspace/cccc/bochs/build/bin/loader.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=3 seek=2 conv=notrunc

# # 定义目录变量
# WORKSPACE=/home/yhy/workspace/cccc/bochs
# BUILD_OBJ=${WORKSPACE}/build/obj
# BUILD_BIN=${WORKSPACE}/build/bin
# SRC_DIR=${WORKSPACE}/src
# LIB_DIR=${SRC_DIR}/lib

# # -fno-builtin
# nasm -f elf -o ${BUILD_OBJ}/print.o ${LIB_DIR}/kernel/print.S
# nasm -f elf -o ${BUILD_OBJ}/kernel.o ${SRC_DIR}/kernel/kernel.S
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/timer.o ${SRC_DIR}/device/timer.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR}/device
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/main.o ${SRC_DIR}/kernel/main.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR}/kernel  -I ${SRC_DIR}/device
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/interrupt.o ${SRC_DIR}/kernel/interrupt.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/init.o ${SRC_DIR}/kernel/init.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/string.o ${LIB_DIR}/string.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device 
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/bitmap.o ${LIB_DIR}/kernel/bitmap.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/debug.o ${SRC_DIR}/kernel/debug.c -I ${LIB_DIR}/kernel -I ${LIB_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/memory.o ${SRC_DIR}/kernel/memory.c -I ${LIB_DIR}/kernel -I ${SRC_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device



# ld -m elf_i386 -Ttext 0xc0001500 -e main -o ${BUILD_BIN}/kernel.bin ${BUILD_OBJ}/main.o ${BUILD_OBJ}/print.o ${BUILD_OBJ}/interrupt.o ${BUILD_OBJ}/init.o ${BUILD_OBJ}/kernel.o ${BUILD_OBJ}/timer.o ${BUILD_OBJ}/debug.o ${BUILD_OBJ}/string.o ${BUILD_OBJ}/memory.o ${BUILD_OBJ}/bitmap.o
# dd if=/home/yhy/workspace/cccc/bochs/build/bin/kernel.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=200 seek=9 conv=notrunc
# nasm -f elf -o ${BUILD_OBJ}/print.o ${LIB_DIR}/kernel/print.S
# gcc -m32 -c -fno-builtin -o ${BUILD_OBJ}/main.o ${SRC_DIR}/kernel/main.c -I ${LIB_DIR}/kernel/ -I ${SRC_DIR}/kernel

# ld -m elf_i386 -Ttext 0xc0001500 -e main -o ${BUILD_BIN}/kernel.bin ${BUILD_OBJ}/main.o ${BUILD_OBJ}/print.o  
# dd if=/home/yhy/workspace/cccc/bochs/build/bin/kernel.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=200 seek=9 conv=notrunc



/home/yhy/workspace/cccc/bochs/bin/bochs -f /home/yhy/workspace/cccc/bochs/src/bochsrc.disk