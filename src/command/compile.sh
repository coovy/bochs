#!/bin/bash
####  此脚本应该在command目录下执行
WORKSPACE=/home/yhy/workspace/cccc/bochs
BUILD_OBJ=${WORKSPACE}/build/obj
BUILD_BIN=${WORKSPACE}/build/bin
SRC_DIR=${WORKSPACE}/src
LIB_DIR=${SRC_DIR}/lib
IMG_DIR=${WORKSPACE}/images
BUILD_DIR=${BUILD_OBJ}

if [[ ! -d "${LIB_DIR}" || ! -d "${BUILD_OBJ}" ]];then
   echo "dependent dir don\`t exist!"
   cwd=$(pwd)
   cwd=${cwd##*/}
   cwd=${cwd%/}
   if [[ $cwd != "command" ]];then
      echo -e "you\`d better in command dir\n"
   fi 
   exit
fi

BIN="prog_arg"
CFLAGS="-Wall -m32 -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers"
LIBS="-I ${LIB_DIR} -I ${LIB_DIR}/user -I ${SRC_DIR}/fs"
OBJS="${BUILD_DIR}/string.o ${BUILD_DIR}/syscall.o ${BUILD_DIR}/stdio.o ${BUILD_DIR}/assert.o ./start.o"
DD_IN=$BIN
DD_OUT="${IMG_DIR}/hd60M.img"

nasm -f elf ./start.S -o ./start.o
ar rcs simple_crt.a $OBJS start.o
gcc $CFLAGS -o $BIN".o" $BIN".c" $LIBS

ld -m elf_i386 $BIN".o" simple_crt.a -o $BIN
SEC_CNT=$(ls -l $BIN|awk '{printf("%d", ($5+511)/512)}')
if [[ -f $BIN ]];then
   dd if=./$DD_IN of=$DD_OUT bs=512 count=$SEC_CNT seek=300 conv=notrunc
fi

##########   以上核心就是下面这五条命令(未修改, 仅作注释)   ##########
#nasm -f elf ./start.S -o ./start.o
#ar rcs simple_crt.a ../build/string.o ../build/syscall.o \
#   ../build/stdio.o ../build/assert.o ./start.o
#gcc -Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes \
#   -Wsystem-headers -I ../lib/ -I ../lib/user -I ../fs prog_arg.c -o prog_arg.o
#ld prog_arg.o simple_crt.a -o prog_arg
#dd if=prog_arg of=/home/work/my_workspace/bochs/hd60M.img \
#   bs=512 count=11 seek=300 conv=notrunc
