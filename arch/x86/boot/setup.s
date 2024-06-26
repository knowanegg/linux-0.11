# ----------------------------------------------------------
# Setup.s
# Maintainer: Buddy <buddy.zhang@aliyun.com>
#
# Copyright (C) 2017 BiscuitOS
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
	.code16
# rewrite with AT&T syntax by falcon <wuzhangjin@gmail.com> at 081012
#
#	setup.s		(C) 1991 Linus Torvalds
#
# setup.s is responsible for getting the system data from the BIOS,
# and putting them into the appropriate places in system memory.
# both setup.s and system has been loaded by the bootblock.
#
# This code asks the bios for memory/disk/other parameters, and
# puts them in a "safe" place: 0x90000-0x901FF, ie where the
# boot-block used to be. It is then up to the protected mode
# system to read them from there before the area is overwritten
# for buffer-blocks.
#

# NOTE! These had better be the same as in bootsect.s!

	.equ INITSEG, 0x9000	# we move boot here - out of the way
	.equ SYSSEG, 0x1000	# system loaded at 0x10000 (65536).
	.equ SETUPSEG, 0x9020	# this is the current segment

	.global _start, begtext, begdata, begbss, endtext, enddata, endbss
	.text
	begtext:
	.data
	begdata:
	.bss
	begbss:
	.text

	ljmp $SETUPSEG, $_start	# 这里又重新跳转了一次，不知道啥情况，为了清缓存？
_start:

# ok, the read went well so we get current cursor position and save it for
# posterity.

	mov	$INITSEG, %ax	# this is done in bootsect already, but...
	mov	%ax, %ds    # 将ds设置为INITSEG：0x9000
	mov	$0x03, %ah	# read cursor pos # 用中断读取指针位置
	xor	%bh, %bh    # 
	int	$0x10		# save it in known place, con_init fetches
	mov	%dx, %ds:0	# 将结果写入0x90000
# Get memory size (extended mem, kB) 获取内存信息
# 这个 BIOS 中断调用返回系统中可用的扩展内存的大小（以 KB 为单位）。
	mov	$0x88, %ah  # 
	int	$0x15
	mov	%ax, %ds:2  

# Get video-card data: 获取显卡数据

	mov	$0x0f, %ah
	int	$0x10
	mov	%bx, %ds:4	# bh = display page
	mov	%ax, %ds:6	# al = video mode, ah = window width

# check for EGA/VGA and some config parameters # 获取显示接口参数

	mov	$0x12, %ah
	mov	$0x10, %bl
	int	$0x10
	mov	%ax, %ds:8
	mov	%bx, %ds:10
	mov	%cx, %ds:12

# Get hd0 data # 获取hd0硬盘的数据

	mov	$0x0000, %ax
	mov	%ax, %ds
	lds	%ds:4*0x41, %si
	mov	$INITSEG, %ax
	mov	%ax, %es
	mov	$0x0080, %di
	mov	$0x10, %cx
	rep
	movsb

# Get hd1 data # 获取hd1硬盘的数据

	mov	$0x0000, %ax
	mov	%ax, %ds
	lds	%ds:4*0x46, %si
	mov	$INITSEG, %ax
	mov	%ax, %es
	mov	$0x0090, %di
	mov	$0x10, %cx
	rep
	movsb

# Check that there IS a hd1 :-) # 看看是否存在hd1

	mov	$0x01500, %ax
	mov	$0x81, %dl
	int	$0x13
	jc	no_disk1
	cmp	$3, %ah
	je	is_disk1

# 在 no_disk1 分支中，通过 rep stosb 在特定的内存区域填充零值
# 这可能是用于初始化或清理内存区域。
no_disk1: 
	mov	$INITSEG, %ax
	mov	%ax, %es
	mov	$0x0090, %di
	mov	$0x10, %cx
	mov	$0x00, %ax
	rep
	stosb
	# STOSB (Store String Byte) 是一条 x86 架构的汇编指令 #
	# 用于将 AL 寄存器中的数据存储到由 ES:DI（在 16 位模式下）
	# 或 RDI（在 64 位模式下）指定的内存地址处，并根据标志寄存器的方向位（DF，方向标志）
	# 自动更新 DI 或 RDI 寄存器的值。
    # 如果 DF 清零（通常情况），DI/RDI 会自增（对于STOSB是加 1，对于STOSW是加 2
	# ，对于STOSD是加 4，对于STOSQ是加 8），允许连续向内存写入数据。
	# 如果 DF 设置为 1，DI/RDI 会自减，允许向低地址连续写入数据。
    # STOSB 指令经常用于内存填充操作，例如，将一块内存区域初始化为特定的值。
	# 由于这条指令可以快速地重复执行，因此它在需要清零或设置大块内存时非常高效。

is_disk1:

# now we want to move to protected mode ...
# 准备进入保护模式

	cli			# no interrupts allowed ! 

# first we move the system to it's rightful place
# 0x10000 > 0x00000 
# 0x20000 > 0x10000
# ......
# 0x90000 > 0x80000
# 往前挪一个0x10000

	mov	$0x0000, %ax
	cld			# 'direction'=0, movs moves forward
do_move:
	mov	%ax, %es	# destination segment
	add	$0x1000, %ax
	cmp	$0x9000, %ax
	jz	end_move
	mov	%ax, %ds	# source segment
	sub	%di, %di    # 清零？这里应该用xor也可以吧
	sub	%si, %si
	mov 	$0x8000, %cx # 循环0x8000次,下面用的是movsw,所以是0x8000*2
                         # 也就是0x10000次，对应前面ax赋值给ds和es的段
	rep
	movsw
	jmp	do_move

# then we load the segment descriptors

# 移动结束，这时候设置idt和gdt
end_move:
	mov	$SETUPSEG, %ax	# right, forgot this at first. didn't work :-)
	mov	%ax, %ds
	lidt	idt_48		# load idt with 0,0
	lgdt	gdt_48		# load gdt with whatever appropriate

# that was painless, now we enable A20

	#call	empty_8042	# 8042 is the keyboard controller
	#mov	$0xD1, %al	# command write
	#out	%al, $0x64
	#call	empty_8042
	#mov	$0xDF, %al	# A20 on
	#out	%al, $0x60
	#call	empty_8042
	inb     $0x92, %al	# open A20 line(Fast Gate A20).
	orb     $0b00000010, %al
	outb    %al, $0x92

# well, that went ok, I hope. Now we have to reprogram the interrupts :-(
# we put them right after the intel-reserved hardware interrupts, at
# int 0x20-0x2F. There they won't mess up anything. Sadly IBM really
# messed this up with the original PC, and they haven't been able to
# rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,
# which is used for the internal hardware interrupts as well. We just
# have to reprogram the 8259's, and it isn't fun.
# 8259重新编程
	mov	$0x11, %al		# initialization sequence(ICW1)
					# ICW4 needed(1),CASCADE mode,Level-triggered
	out	%al, $0x20		# send it to 8259A-1
	.word	0x00eb,0x00eb		# jmp $+2, jmp $+2
	out	%al, $0xA0		# and to 8259A-2
	.word	0x00eb,0x00eb
	mov	$0x20, %al		# start of hardware int's (0x20)(ICW2)
	out	%al, $0x21		# from 0x20-0x27
	.word	0x00eb,0x00eb
	mov	$0x28, %al		# start of hardware int's 2 (0x28)
	out	%al, $0xA1		# from 0x28-0x2F
	.word	0x00eb,0x00eb		#               IR 7654 3210
	mov	$0x04, %al		# 8259-1 is master(0000 0100) --\
	out	%al, $0x21		#				|
	.word	0x00eb,0x00eb		#			 INT	/
	mov	$0x02, %al		# 8259-2 is slave(       010 --> 2)
	out	%al, $0xA1
	.word	0x00eb,0x00eb
	mov	$0x01, %al		# 8086 mode for both
	out	%al, $0x21
	.word	0x00eb,0x00eb
	out	%al, $0xA1
	.word	0x00eb,0x00eb
	mov	$0xFF, %al		# mask off all interrupts for now
	out	%al, $0x21
	.word	0x00eb,0x00eb
	out	%al, $0xA1

# well, that certainly wasn't fun :-(. Hopefully it works, and we don't
# need no steenking BIOS anyway (except for the initial loading :-).
# The BIOS-routine wants lots of unnecessary data, and it's less
# "interesting" anyway. This is how REAL programmers do it.
#
# Well, now's the time to actually move into protected mode. To make
# things as simple as possible, we do no register set-up or anything,
# we let the gnu-compiled 32-bit programs do that. We just jump to
# absolute address 0x00000, in 32-bit protected mode.
	#mov	$0x0001, %ax	# protected mode (PE) bit
	#lmsw	%ax		# This is it!
	mov	%cr0, %eax	# get machine status(cr0|MSW)	
	bts	$0, %eax	# 将cr0最后一位置为1，打开保护模式 bts（Bit Test and Set）。它会测试指定位置的位并设置该位。
	mov	%eax, %cr0	# protection enabled
				
				# segment-descriptor        (INDEX:TI:RPL)
	.equ	sel_cs0, 0x0008 # select for code segment 0 (  001:0 :00)
                            # 段选择子0x0008，最后三位是标志位，也就是第一个段
							#   15                           3   2   1 0
                            # +--------------------------------+---+----+
                            # |          索引 (Index)          | TI | RPL |
                            # +--------------------------------+---+----+
	ljmp	$sel_cs0, $0	# jmp offset 0 of code segment 0 in gdt
	                        # 跳到第一个段的0x0执行，也是就是系统加载段

# This routine checks that the keyboard command queue is empty
# No timeout is used - if this hangs there is something wrong with
# the machine, and we probably couldn't proceed anyway.
empty_8042:
	.word	0x00eb,0x00eb
	in	$0x64, %al	# 8042 status port
	test	$2, %al		# is input buffer full?
	jnz	empty_8042	# yes - loop
	ret

gdt:
	.word	0,0,0,0		# dummy

	.word	0x07FF		# 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		# base address=0 # 跳到0x00000执行，也就是我们吧system搬到的地方
	.word	0x9A00		# code read/exec
	.word	0x00C0		# granularity=4096, 386

	.word	0x07FF		# 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		# base address=0
	.word	0x9200		# data read/write
	.word	0x00C0		# granularity=4096, 386

idt_48:
	.word	0			# idt limit=0
	.word	0,0			# idt base=0L

gdt_48:
	.word	0x800			# gdt limit=2048, 256 GDT entries gdt界限
	.word   512+gdt, 0x9		# gdt base = 0X9xxxx,         gdt地址
	# 512+gdt is the real gdt after setup is moved to 0x9020 * 0x10
	
.text
endtext:
.data
enddata:
.bss
endbss:
