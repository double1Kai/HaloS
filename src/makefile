BUILD:=../build
SRC:=.

MULTIBOOT2:=0x10000
ENTRYPOINT:=$(shell python -c "print(f'0x{$(MULTIBOOT2) + 64:x}')")

CFLAGS:= -m32 #32位程序
CFLAGS+= -fno-builtin 	#不需要gcc内置函数
CFLAGS+= -nostdinc 		#不需要标准头文件 
CFLAGS+= -fno-pic 		#不需要位置无关的代码(postion independent code) 
CFLAGS+= -fno-pie 		#不需要位置无关的可执行程序(postion independent excutable) 
CFLAGS+= -nostdlib 		#不需要标准库
CFLAGS+= -fno-stack-protector 	#不需要栈保护
CFLAGS:=$(strip ${CFLAGS}) 		#让命令行更好看点
DEBUG:= -g
INCLUDE:=-I$(SRC)/include

$(BUILD)/boot/%.bin: $(SRC)/boot/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $(DEBUG) $< -o $@

$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

LDFLAGS:= -m elf_i386 \
		-static \
		-Ttext $(ENTRYPOINT) \
		--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})

$(BUILD)/kernel.bin: \
	$(BUILD)/kernel/start.o \
	$(BUILD)/kernel/main.o \
	$(BUILD)/kernel/io.o \
	$(BUILD)/kernel/console.o \
	$(BUILD)/kernel/printk.o \
	$(BUILD)/kernel/assert.o \
	$(BUILD)/kernel/debug.o \
	$(BUILD)/kernel/global.o \
	$(BUILD)/kernel/device.o \
	$(BUILD)/kernel/task.o \
	$(BUILD)/kernel/gate.o \
	$(BUILD)/kernel/schedule.o \
	$(BUILD)/kernel/interrupt.o \
	$(BUILD)/kernel/handler.o \
	$(BUILD)/kernel/clock.o \
	$(BUILD)/kernel/time.o \
	$(BUILD)/kernel/rtc.o \
	$(BUILD)/kernel/ide.o \
	$(BUILD)/kernel/buffer.o \
	$(BUILD)/kernel/memory.o \
	$(BUILD)/kernel/bitmap.o \
	$(BUILD)/kernel/list.o \
	$(BUILD)/kernel/thread.o \
	$(BUILD)/kernel/mutex.o \
	$(BUILD)/kernel/keyboard.o \
	$(BUILD)/kernel/fifo.o \
	$(BUILD)/kernel/arena.o \
	$(BUILD)/fs/super.o \
	$(BUILD)/fs/bmap.o \
	$(BUILD)/fs/inode.o \
	$(BUILD)/fs/namei.o \
	$(BUILD)/lib/vsprintf.o \
	$(BUILD)/lib/string.o \
	$(BUILD)/lib/printf.o \
	$(BUILD)/lib/syscall.o \
	$(BUILD)/lib/stdlib.o
	
	$(shell mkdir -p $(dir $@))
	ld ${LDFLAGS} $^ -o $@ -Ttext $(ENTRYPOINT)

$(BUILD)/system.bin: $(BUILD)/kernel.bin
	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
	$(BUILD)/boot/loader.bin \
	$(BUILD)/system.bin \
	$(BUILD)/system.map \
	$(SRC)/utils/master.sfdisk \

	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
	# 测试system.bin的大小，大于100k的话需要写入更多，lodaer里也需要读入更多
	test -n "$$(find $(BUILD)/system.bin -size -100k)"
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

	# 对硬盘进行分区
	sfdisk $@ < $(SRC)/utils/master.sfdisk
	# 挂载设备
	sudo losetup /dev/loop0 --partscan $@
	# 创建文件系统，第一版minix，文件名长度14字节，在loop0设备的第一个分区
	sudo mkfs.minix -1 -n 14 /dev/loop0p1
	# 挂载文件系统
	sudo mount /dev/loop0p1 /mnt
	# 切换所有者
	sudo chown ${USER} /mnt
	# 创建目录，home和四级目录
	mkdir -p /mnt/home
	mkdir -p /mnt/d1/d2/d3/d4
	# 创建一些文件
	echo "hello halos!!!, from root direcotry file ..." > /mnt/hello.txt
	echo "hello halos!!!, from home direcotry file ..." > /mnt/home/hello.txt
	# 卸载文件系统
	sudo umount /mnt
	# 卸载设备
	sudo losetup -d /dev/loop0

	
$(BUILD)/slave.img:
	#创建一个32M的磁盘镜像
	yes | bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@
	# 对硬盘进行分区
	sfdisk $@ < $(SRC)/utils/slave.sfdisk
	# 挂载设备
	sudo losetup /dev/loop0 --partscan $@
	# 创建文件系统，第一版minix，文件名长度14字节，在loop0设备的第一个分区
	sudo mkfs.minix -1 -n 14 /dev/loop0p1
	# 挂载文件系统
	sudo mount /dev/loop0p1 /mnt
	# 切换所有者
	sudo chown ${USER} /mnt
	# 创建一些文件
	echo "hello halos!!!, from root direcotry file ..." > /mnt/hello.txt
	# 卸载文件系统
	sudo umount /mnt
	# 卸载设备
	sudo losetup -d /dev/loop0

.PHONY: mount0
mount0: $(BUILD)/master.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown $(USER)	/mnt

.PHONY: umount0
umount0: /dev/loop0
	-sudo umount /mnt
	-sudo losetup -d $<

.PHONY: mount1
mount1: $(BUILD)/slave.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown $(USER)	/mnt

.PHONY: umount1
umount1: /dev/loop1
	-sudo umount /mnt
	-sudo losetup -d $<

IMAGES:= $(BUILD)/master.img $(BUILD)/slave.img

image: $(IMAGES)










$(BUILD)/kernel.iso : $(BUILD)/kernel.bin $(SRC)/utils/grub.cfg
#检测内核文件是否合法
	grub-file --is-x86-multiboot2 $<
#创建iso目录
	mkdir -p $(BUILD)/iso/boot/grub
#拷贝内核文件
	cp $< $(BUILD)/iso/boot
#拷贝grub配置
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub
#生成iso文件
	grub-mkrescue -o $@ $(BUILD)/iso

test: $(BUILD)/master.img

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs -q

.PHONY: bochsb
bochsb: $(BUILD)/kernel.iso
	bochs -q -f bochsrc.grub -unlock

QEMU:= qemu-system-i386 \
	-m 32M \
	-audiodev pa,id=hda \
	-machine pcspk-audiodev=hda \
	-rtc base=localtime \

QEMU_DISK:=-boot c \
	-drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw \
	-drive file=$(BUILD)/slave.img,if=ide,index=1,media=disk,format=raw

QEMU_CDROM:=-boot d \
	-drive file=$(BUILD)/kernel.iso,media=cdrom \

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(IMAGES)
	$(QEMU) $(QEMU_DISK)

.PHONY: qemug
qemug: $(IMAGES)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

.PHONY: qemub
qemub: $(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM) \
	# $(QEMU_DEBUG)

$(BUILD)/master.vmdk: $(BUILD)/master.img
	qemu-img convert -pO vmdk $< $@

.PHONY: vmdk
vmdk: $(BUILD)/master.vmdk