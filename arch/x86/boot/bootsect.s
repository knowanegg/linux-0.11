	.code16
# rewrite with AT&T syntax by falcon <wuzhangjin@gmail.com> at 081012
# modify for biscuitos by buddy <buddy.zhang@aliyun.com> at 170303
#
# SYS_SIZE is the number of clicks (16 bytes) to be loaded.
# 0x3000 is 0x30000 bytes = 196KB, more than enough for current
# version of linux
	.equ SYSSIZE, 0x3000

#
#	bootsect.s		(C) 1991 Linus Torvalds
#
# bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
# iself out of the way to address 0x90000, and jump there.
#
# It then load's 'setup' directly after itself (0x90200), and the system
# at 0x10000, using BIOS interrupts.
#
# NOTE! currently system is at most 8*65536 bytes long. This should be no
# problem, even in the future. I want to keep it simple. This 512 kB
# kernel size should by enough, especially as this doesn't contain the 
# buffer cache as in minix
#
# The loader has been make as simple as possible, and continuos
# read errors will result in a unbreakable loop. Reboot by hand. It
# loads pretty fast by getting whole sectors at a time whenever possible.

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

	.equ SETUPLEN, 4        # nr of setup-sectors
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

	# Normalize the start address
	ljmp $BOOTSEG, $_start

_start:
	# 打断点0x7c00，停在这条指令，为啥？
	# 因为计算机启动第一条指令是FFFF:0000，这条指令指向BIOS的ROM 0xFFFF0
	# 经过BIOS自检后，将硬盘0磁道0扇区的数据加载到这里
	mov $BOOTSEG, %ax
	# BOOTSEG是定义的常量0x7c00，也就是现在的位置，现在是0000:7c00，要使用段寄存器就变成07c0:0000
	# 要把现在的程序256字节移动到INITSEG也就是0x9000段处，为什么？要腾出空间
	mov %ax, %ds
	mov $INITSEG, %ax
	mov %ax, %es
	mov $256, %cx
	sub %si, %si
	sub %di, %di
	rep
	movsw
	# long jump 长转移指令 这里$go是下面go:的偏移，加上$INITSEG就是9000:go，程序挪过去的位置
	ljmp $INITSEG, $go
go:
	# 将cs值赋给ds、es、ss
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
# put stack at 0x9ff00
	mov %ax, %ss
	# 栈指针设置在0xFF00，这时栈位于9000:ff00也就是0x9ff00
	mov $0xFF00, %sp # arbitrary value >> 512 ？这里不知道为啥

# load the setup-sectors directly after the bootblock
# Note that 'es' is already set up

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
	mov %ax, %ax
	mov %ax, %ax
	# 这两行代码实际上没有执行任何操作，它们等效于 NOP（无操作）。可能是为了对齐、占位或其他特定编码习惯。
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
	int $0x10          # 调用 0x10中断

# Ok, we've written the message, now
# we want to load the system (at 0x10000)

	mov $SYSSEG, %ax
	mov %ax, %es    # Segment of 0x010000
	call read_it
	call kill_motor

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
	mov %ax, %cs:root_dev+0

# after that (everything loaded), we jump to
# the setup-routine loaded directly after
# the bootblock:
	mov %ax, %ax
	mov %bx, %bx
	mov %bx, %bx
	ljmp $SETUPSEG, $0

# This routine loads the system at address 0x10000, making sure
# no 64kB boundaries are crossed. We try to load it as fast as
# possible, loading whole tracks whenever we can.
#
# in:   es - starting address segment (normally 0x1000)
#
sread:	.word 1+ SETUPLEN  # sectors read of current track
head:	.word 0			   # current head
track:	.word 0			   # current track

read_it:
	mov %es, %ax
	test $0x0fff, %ax
die:
	jne die       # es must be at 64kB boundary
	xor %bx, %bx  # bx is starting address with segment
rp_read:
	mov %es, %ax
	cmp $ENDSEG, %ax   # have we loaded all yet?
	jb ok1_read
	ret

ok1_read:
	#seg cs
	mov %cs:sectors+0, %ax
	sub sread, %ax
	mov %ax, %cx
	shl $9, %cx
	add %bx, %cx
	jnc ok2_read     # don't over 64KB
	je ok2_read
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
	.word ROOT_DEV
	.word 0xAA55

	.text
	endtext:
	.data
	enddata:
	.bss
	endbss:
