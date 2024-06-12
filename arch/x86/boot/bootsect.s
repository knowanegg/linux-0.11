# 从bios跳转到这里，也就是0x7c00开始执行
# 这里开始就是0x7c00不需要在编译设置偏移
# 是因为BIOS已经把硬盘0扇区也就是这个程序
# 挪到了0x7c00，然后在BIOS最后一条指令
# jmp到了0x7c00

# 编译完后，用objdump看vmlinux，开头是head.s
# objdump -d -m i386 ./vmlinux --start-address=0x0 --stop-address=0x400 
# 那么这个bootsect和setup在哪呢？
# 应该是在硬盘1、2扇区，而vmlinux被写入到了
# 从第三个扇区开始
# 我们想找到真的编译成果，就去找硬盘文件
# 看makefile，实际编译出来的合并到了bootloader
# objdump -D -b binary -m i386 ./arch/x86/boot/bootloader

# 第一步，先把自己移动到0x90000，为后面
# 腾出空间

# 首先通过ax设置es到0x9000
# 注意这里还没有进入保护模式，所以现在还是
# 用ax，16位，也不用movl而是mov
.code16 # 设置编译成16位指令
	.global _start
# 为什么一定要_start?
# _start相当于c语言的main！必须存在的入口
# 链接器会找_start,名称不能变
	.global _start, begtext, begdata, begbss, endtext, enddata, endbss

	.text
	begtext:
	.data
	begdata:
	.bss
	begbss:
	.text


	# BiscuitOS support boot from floppy and hard disk
	# Boot from first hard disk
	# .equ DEVICE_NR, 0x80
	# Boot from first floppy
	.equ DEVICE_NR, 0x00
    .equ SYSSIZE, 0x3000
	.equ SETUPLEN, 4        # nr of setup-sectors setup占用的扇区数
	.equ BOOTSEG, 0x07C0    # original address of boot-sector
	.equ INITSEG, 0x9000    # we move boot here - out of the way
	.equ SETUPSEG, 0x9020   # setup starts here
	.equ SYSSEG, 0x1000     # system loaded at 0x10000 (65536).
	.equ ENDSEG, SYSSEG + SYSSIZE # where to stop loading

# ROOT_DEV: rootfs
# Device number of Rootfs. define as:
# Device number = major * 256 + minor
# dev_no = (major << 8) + minor
# Major device number:
# Memory   : 1
# floppy   : 2
# Disk     : 3
# ttyx     : 4
# tty      : 5
# parallel : 6
# non-pipo : 7
# 0x300    : /dev/hd0 - The first hard disk
# 0x301    : /dev/hd1 - The first partition on first hard disk.
# 0x302    : /dev/hd2 - The second partition on first hard disk.
# ...
# 0x304    : /dev/hd4 - The second hard disk
# 0x305    : /dev/hd5 - The first partition on second hard disk.
# 0x306    : /dev/hd6 - The second partition on second hard disk.
# ROOT_DEV = 0 ; The same device with boot device.
	.equ ROOT_DEV, 0x301
_start:
    mov $0x9000, %ax
    mov %ax, %es
# 现在设置好了es目标段，设置当前段ds
    mov $0x07c0, %ax
    mov %ax, %ds
# 循环移动本身,大小为512也就是0x200大小
# 用movsw可以一次移动两个byte，除以二=0x100=256
    mov $256, %cx
# 清理si和di
    xor %si, %di
# 循环拷贝
    rep movsw

# 拷贝完后，跳转到新地址执行
# 这里有的是jmpf，有的是ljmp，看不同编译器
# ljmp seg, ip
    ljmp $0x9000, $new_start

new_start:
# 重设段寄存器
	# 将cs值赋给ds、es、ss
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
# put stack at 0x9ff00
	mov %ax, %ss
	# 栈指针设置在0xFF00，这时栈位于9000:ff00也就是0x9ff00
	mov $0xFF00, %sp # arbitrary value >> 512 ？这里不知道为啥注释这个
	
# CHS (Cylinder-Head-Sector) 寻址方式在BIOS中断调用中使用。柱面（Cylinder）和磁头（Head）编号从0开始，但扇区（Sector）编号从1开始。
# load the setup-sectors directly after the bootblock
# Note that 'es' is already set up
# 读取setup扇区，用BIOS的0x13中断调用
load_setup:
	# 读硬盘，调用中断处理程序
	# If use hard disk, dirver is 0x80 
	mov $0x0000, %dx      # head 0
	mov $DEVICE_NR, %dl   # drive 0 
	mov $0x0002, %cx   # sector 2 扇区（Sector）, track 0 磁道（Track）这里为什么是2扇区
                       # 因为第一扇区是boot，也就是现在的程序，紧跟着的下一个扇区就是setup
	mov $0x0200, %bx   # address = 512, in INITSEG 加载到INITSEG的512地址处
	.equ     AX, 0x200+SETUPLEN # 这里的equ不是判断，是设置常量相当于 #define AX 0x200+SETUPLEN
	mov     $AX, %ax   # service 2, nr of sectors
	int $0x13          # read it 调用0x13中断处理，这是实模式下BIOS提供的磁盘服务中断
	jnc ok_load_setup  # jnc 是一个汇编语言指令，代表 "Jump if Not Carry"。
					   # 条件跳转，用于在没有进位（Carry Flag未设置）的情况下跳转到指定的地址。
					   # 这里如果没有进位，CF=0，说明读取setup程序成功，跳转到ok_load_setup
					   # 如果CF=1，表示有错误，将通过下面的代码重置磁盘，然后重新读取
	mov $0x0000, %dx   #
	mov $DEVICE_NR, %dl #
	mov $0x0000, %ax   # reset the diskette
	int $0x13          # 再次调用中断处理程序，重置磁盘
	jmp load_setup     # 跳转到开头重新读

ok_load_setup:

# Get disk drive parameters, specifically nr of sectors/track
# 获取磁盘参数，特别是磁道数和扇区数

	mov $DEVICE_NR, %dl # 指定设备号
	mov $0x0800, %ax # AH 被设置为 0x08，代表获取磁盘驱动器参数的功能
	int $0x13 # %cx是系统调用返回值
              # CL: 扇区数的低6位和柱面数的高2位（柱面号的高2位在第6和第7位，扇区号在低6位）
	mov $0x00, %ch
	
	#seg cs
	mov %cx, %cs:sectors+0 # sectors在最后面定义，这里不是sectors的内容而是在本程序中的偏移。
                           # 这条语句是将%cx的内容写入到sectors位置定义的word中
	mov $INITSEG, %ax # 将附加段指向INITSEG
	mov %ax, %es 

# Print some iname message
# 打印信息

	mov $0x03, %ah     # 读取指针位置
	xor %bh, %bh
	int $0x10          # 调用0x10中断

	mov $27, %cx       # 要显示27个字符
	mov $0x0007, %bx   # page 0, attribute 7 (normal) 显示属性
#	lea msg1, %bp
	mov $msg1, %bp     # message存储位置放入bp
	mov $0x1301, %ax   
	int $0x10          # 调用 0x10中断,在屏幕上显示message

# Ok, we've written the message, now
# we want to load the system (at 0x10000)

	mov $SYSSEG, %ax # 将要加载系统的段放入es,这里SYSSGE是 .equ SYSSEG, 0x1000
	mov %ax, %es    # Segment of 0x010000
	call read_it    # 读取方法
	call kill_motor # 

# After that we check which root-device to use. If the device is
# defined (#= 0), nothing is done and the given device is used.
# Otherwise, either /dev/PS0 (2,28) or /dev/at0 (2,8), depending
# on the number of sectors that the BIOS report currently.

	#seg cs
	mov %cs:root_dev+0, %ax
	cmp $0, %ax
	jne root_defined
	#seg cs
	mov %cs:sectors+0, %bx
	mov $0x0208, %ax        # /dev/ps0 - 1.2Mb
	cmp $15, %bx
	je root_defined
	mov $0x021c, %ax        # /dev/PS0 - 1.44Mb
	cmp $18, %bx
	je root_defined
undef_root:
	jmp undef_root
root_defined:
	#seg cs
	mov %ax, %cs:root_dev+0 # 如果一开始就define了，那么就相当于来回mov了一遍，没作用

# after that (everything loaded), we jump to
# the setup-routine loaded directly after
# the bootblock:
	ljmp $SETUPSEG, $0  # 最后一条指令，跳转到setup.s

# This routine loads the system at address 0x10000, making sure
# no 64kB boundaries are crossed. We try to load it as fast as
# possible, loading whole tracks whenever we can.
#
# in:   es - starting address segment (normally 0x1000)
#
sread:	.word 1+ SETUPLEN  # sectors read of current track
head:	.word 0			   # current head
track:	.word 0			   # current track
# 下面一大串基本就是read硬盘操作，将vmlinux文件读到内存0x10000处，很繁杂，不用看
read_it:
	mov %es, %ax       # 比较es和0x0fff，从read SYSSEG来的话es应该是0x1000
	test $0x0fff, %ax  # test指令执行的是位与（AND）操作，但结果不会保存，仅用来影响标志位。
die:
	jne die       # es must be at 64kB boundary es必须64K对齐，如果=0x0fff有对上的说没对齐
				  # 然后自己跳转到自己，无限死循环？
	xor %bx, %bx  # bx is starting address with segment 这里清零
rp_read:
	mov %es, %ax
	cmp $ENDSEG, %ax   # have we loaded all yet? # 看看有没有完全加载完
	jb ok1_read        # jb（Jump if Below）
                       # 如果%ax中的值（无符号数）小于$ENDSEG的值，就跳转到ok1_read
	ret	               # 否则就说明已经读完，ret到166行

ok1_read:
	#seg cs
	mov %cs:sectors+0, %ax # 这里的sectors是前面142行通过系统调用读到的
	sub sread, %ax   # sread:.word 1+ SETUPLEN  # sectors read of current track
                     # 这里要先把本程序(boot)占用的1和setup程序占用的扇区数减掉
	mov %ax, %cx     # 剩下的扇区数放入cx进行循环
	shl $9, %cx      # 左移<9位,相当于*2^9=512，每个扇区的字节数
	add %bx, %cx     # 加上bx，段内起始地址，
	jnc ok2_read     # Jump if Not Carry 没有进位就跳转 don't over 64KB
	je ok2_read      # 
	xor %ax, %ax     # over 64KB
	sub %bx, %ax
	shr $9, %ax
ok2_read:
	call read_track
	mov  %ax, %cx
	add sread, %ax
	#seg cs
	cmp %cs:sectors+0, %ax
	jne ok3_read
	mov $1, %ax
	sub head, %ax
	jne ok4_read
	incw track
ok4_read:
	mov %ax, head
	xor %ax, %ax
ok3_read:
	mov %ax, sread
	shl $9, %cx
	add %cx, %bx
	jnc rp_read
	mov %es, %ax
	add $0x1000, %ax
	mov %ax, %es
	xor %bx, %bx
	jmp rp_read

# Read a track data
# AL: the number of read sector
# [BX:ES]: Store data
read_track:
	push %ax
	push %bx
	push %cx
	push %dx
	mov track, %dx
	mov sread, %cx
	inc %cx             # Sectors
	mov %dl, %ch        # Cylinders
	mov head, %dx
	mov %dl, %dh        # Heads
	mov $DEVICE_NR, %dl
	and $0x0100, %dx  # boot from floppy
	mov $2, %ah
	int $0x13
	jc bad_rt
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	ret
bad_rt:
	mov $0, %ax
	mov $0, %dx
	int $0x13
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	jmp read_track

#
# This procedure turns off the floppy dirve motor, so
# that we enter the kernel a known state, and
# don't have to worry about it later.
kill_motor:
	push %dx
	mov $0x3f2, %dx
	mov $0, %al
	outsb
	pop %dx
	ret

sectors:
	.word 0

msg1:
	.byte 13,10
	.ascii "Loading BiscuitOS ..."
	.byte 13,10,13,10

	.org 508
root_dev:
	.word ROOT_DEV # .equ ROOT_DEV, 0x301 /dev/hd1
	.word 0xAA55

	.text
	endtext:
	.data
	enddata:
	.bss
	endbss:
