# Record some commands for ease of use

nasm -I /home/yhy/workspace/cccc/bochs/include -o /home/yhy/workspace/cccc/bochs/bin2/loader.bin /home/yhy/workspace/cccc/bochs/src/loader.S

dd if=/home/yhy/workspace/cccc/bochs/bin2/loader.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=2 seek=2 conv=notrunc

dd if=/home/yhy/workspace/cccc/bochs/bin2/mbr.bin of=/home/yhy/workspace/cccc/bochs/images/hd60M.img bs=512 count=1 conv=notrunc

/home/yhy/workspace/cccc/bochs/bin/bochs -f /home/yhy/workspace/cccc/bochs/src/bochsrc.disk