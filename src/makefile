WORKSPACE=/home/yhy/workspace/cccc/bochs
BUILD_OBJ=${WORKSPACE}/build/obj
BUILD_BIN=${WORKSPACE}/build/bin
SRC_DIR=${WORKSPACE}/src
LIB_DIR=${SRC_DIR}/lib


BUILD_DIR = ${BUILD_OBJ}
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I ${LIB_DIR}/kernel -I ${LIB_DIR}/user -I ${LIB_DIR} -I ${SRC_DIR} -I ${SRC_DIR}/kernel -I ${SRC_DIR}/device
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -m32 -c -fno-builtin -W -Wstrict-prototypes \
         -Wmissing-prototypes 
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
# OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
#       $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
#       $(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o \
#       $(BUILD_DIR)/string.o 
OBJS = $(BUILD_DIR)/*.o

##############     c代码编译     ###############
$(BUILD_DIR)/main.o: ${SRC_DIR}/kernel/main.c ${SRC_DIR}/lib/kernel/print.h \
        ${SRC_DIR}/lib/stdint.h ${SRC_DIR}/kernel/init.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/init.o: ${SRC_DIR}/kernel/init.c ${SRC_DIR}/kernel/init.h ${SRC_DIR}/lib/kernel/print.h \
        ${SRC_DIR}/lib/stdint.h ${SRC_DIR}/kernel/interrupt.h ${SRC_DIR}/device/timer.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/interrupt.o: ${SRC_DIR}/kernel/interrupt.c ${SRC_DIR}/kernel/interrupt.h \
        ${SRC_DIR}/lib/stdint.h ${SRC_DIR}/kernel/global.h ${SRC_DIR}/lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/timer.o: ${SRC_DIR}/device/timer.c ${SRC_DIR}/device/timer.h ${SRC_DIR}/lib/stdint.h\
        ${SRC_DIR}/lib/kernel/io.h ${SRC_DIR}/lib/kernel/print.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/debug.o: ${SRC_DIR}/kernel/debug.c ${SRC_DIR}/kernel/debug.h \
        ${SRC_DIR}/lib/kernel/print.h ${SRC_DIR}/lib/stdint.h ${SRC_DIR}/kernel/interrupt.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/string.o: ${SRC_DIR}/lib/string.c ${SRC_DIR}/lib/string.h ${SRC_DIR}/lib/stdint.h kernel/global.h \
	${SRC_DIR}/lib/stdint.h ${SRC_DIR}/kernel/debug.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/bitmap.o: ${SRC_DIR}/lib/kernel/bitmap.c ${SRC_DIR}/lib/kernel/bitmap.h \
    	${SRC_DIR}/kernel/global.h ${SRC_DIR}/lib/stdint.h ${SRC_DIR}/lib/string.h ${SRC_DIR}/lib/stdint.h \
     	${SRC_DIR}/lib/kernel/print.h ${SRC_DIR}/kernel/interrupt.h ${SRC_DIR}/kernel/debug.h
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/memory.o: ${SRC_DIR}/kernel/memory.c ${SRC_DIR}/kernel/memory.h ${SRC_DIR}/lib/stdint.h ${SRC_DIR}/lib/kernel/bitmap.h \
   	${SRC_DIR}/kernel/global.h ${SRC_DIR}/kernel/global.h ${SRC_DIR}/kernel/debug.h ${SRC_DIR}/lib/kernel/print.h \
	${SRC_DIR}/lib/kernel/io.h ${SRC_DIR}/kernel/interrupt.h ${SRC_DIR}/lib/string.h ${SRC_DIR}/lib/stdint.h
	$(CC) $(CFLAGS) $^ -o $@

##############    汇编代码编译    ###############
$(BUILD_DIR)/kernel.o: ${SRC_DIR}/kernel/kernel.S
	$(AS) $(ASFLAGS) $^ -o $@
$(BUILD_DIR)/print.o: ${SRC_DIR}/lib/kernel/print.S
	$(AS) $(ASFLAGS) $^ -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all

mk_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

hd:
	dd if=$(BUILD_DIR)/kernel.bin \
           of=/home/work/my_workspace/bochs/hd60M.img \
           bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd