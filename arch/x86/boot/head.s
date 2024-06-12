/* 
 * head.s
 *
 * Copyright (C) 1991 Linus Torvalds
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * head.s contains the 32-bit startup code.
 *
 * NOTE!!! Startup happens at absolute address 0x00000000, which is also
 * where the page directory will exist. The startup code will be
 * overwritten by the page directory.
 */

	.text
	.globl idt, gdt, pg_dir, tmp_floppy_area

pg_dir:
	.globl startup_32

startup_32:
	movl $0x10, %eax  # 0x10, Global data segment 010:0:00 gdt中第二条
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	lss stack_start, %esp # “Load Full Pointer to SS”
	                      # 即“加载完整指针到SS寄存器”
						  # 这个指令同时更新SS和ESP寄存器。
	call setup_idt
	call setup_gdt
	movl $0x10, %eax         # Reload all the segment registers
	mov %ax, %ds             # after changing gdt. CS was already
	mov %ax, %es             # reloaded in 'setup_gdt'
	mov %ax, %fs
	mov %ax, %gs
	lss stack_start, %esp   # stack_start 是一个内存地址，指向包含两个部分的数据结构：
	                        # 新的堆栈指针值和新的堆栈段选择子。首先是堆栈指针值（低地址处）
							# 其次是段选择子（高地址处）。低地址明显看出给了%esp
							# 高地址隐式赋值给了SS(隐式的意思是这条指令没出现ss)
	xorl %eax, %eax
1:
	incl %eax                # Check that A20 really is enabled
	movl %eax, 0x000000      # loop forever if it isn't
	cmpl %eax, 0x100000
	je 1b

/*
 * NOTE! 486 should set bit 16, to check for write-protect in
 * supervisor mode. Then it would be unnecessary with the
 * "verify_area()" -calls. 486 users probably want to set the
 * NE (#5) bit also, so as to use int 16 for math errors.
 */
 # cr0寄存器，设置权限
	movl %cr0, %eax          # Check math chip
	andl $0x80000011, %eax   # Save PG, PE, ET ，注意这里是and不是or，没开的还是没开，pg这里没开
	                         # PE（Protection Enable）就是是否保护模式
							 # 曾经试验过，保护模式运行一半关了，能退回实模式
							 # PG（Paging）是否开启分页机制，有了分页就有了虚拟内存
							 # 不然只能直接操作真实内存地址
							 # ET（Extension Type）80286遗留不怎么用
	/* "orl $0x10020, %eax" here for 486 might be good */
	orl $2, %eax             # set MP
	                         # MP（Monitor coProcessor）允不允许浮点和协处理器
	movl %eax, %cr0
	call check_x87           # 看看有没有8087这种协处理器
	jmp after_page_tables

/*
 * We depend on ET to be correct. This checks for 286/387.
 */
 # FNINIT指令用于初始化浮点单元（FPU）的状态。这条指令不需要任何操作数，
 # 它将FPU的控制字（Control Word）、状态字（Status Word）、标记字（Tag Word）
 # 等寄存器重置到它们的默认值。
check_x87:
	fninit
	fstsw %ax
	cmpb $0, %al
	je 1f              /* no coprocessor: have to set bits */
	movl %cr0, %eax
	xorl $6, %eax      /* reset MP, set EM */
	movl %eax, %cr0
	ret

.align 2
1:
	.byte 0xDB, 0xE4    /* fsetpm for 287, ignored by 387 */
	                    # 287操作码，自动被387忽略
	ret

/*
 * setup_idt
 *
 * Sets up IDT with 256 entries pointing to
 * ignore_int, interrupt gates. It then loads IDT.
 * Everything that wants to install itself in the
 * idt-table may do so themselves. Interrupts
 * are enabled elsewhere, when we can be relatively
 * sure everthing is ok. This routine will be
 * over-written by the page tables.
 */ # ignore_int 忽略interrupt
setup_idt:
	lea ignore_int, %edx    # 全称为“Load Effective Address”
	                        # 即“加载有效地址”。这个指令计算内存地址表达式的值
							# 但不是将该地址处的数据加载到寄存器中，
							# 而是将计算出的地址值本身加载到目标寄存器中。
							# 即使是lea ax,[bx+4]，也不读内存，只是ax=bx+4
							# 这里进入保护模式了，操作数顺序要倒过来看a->b
	movl $0x00080000, %eax  # movl long 四字节
	movw %dx, %ax           # movw word 两字节
	                        # dx覆盖ax低16位0x0008_dx
                            # 也就是ignore_int的程序内偏移

	movw $0x8E00, %dx       # dx设置为0x8E00

	lea idt, %edi           # 将idt地址放入edi，也就是目的地址
                            # 那接下来写的就是idt表了
	mov $256, %ecx          # 循环256次
rp_sidt:
	movl %eax, (%edi)       # eax值是0x80000加ign_int偏移,传给es:di
	                        # 也就是写到idt表里。
	movl %edx, 4(%edi)      # edx值是0x8e00，传给es:di+4。高位
	                        # 为什么加0x80000?
							# 这里就是idt表的格式了
							
							# 0x00008e00
							# 0x00085428 

# 31                               16 15                                0
# +------------------+-----------------+----------------+----------------+
# |  Offset High     |  Type & Attr    |  Segment Selector | Offset Low   |
# +------------------+-----------------+----------------+----------------+

                             # 这里段选择符是0x0008，偏移是0x5428
							 # 高位偏移0x0000，属性是0x8e00

#  15 14 13  12  11  10  9  8  7  6  5  4  3  2  1  0
# +---------------------------------------------------+
# | P | DPL | S |          Type                       |
# +---------------------------------------------------+
# | 1 | 0 0 | 0 | 1  1  1  0 | 0  0  0  0  0  0  0  0 |
# +---------------------------------------------------+
                             # Present是否存在
							 # DPL权限 00表示该描述符可以被任意特权级的代码访问。
                             # S (System): 第12位，为0，标示这是一个系统段。
							 # Type: 第11至第8位为1110，标识这是一个32位的中断门
							 # 最后几位是保留，没啥意义

	addl $8, %edi
	dec %ecx
	jne rp_sidt
	lidt idt_descr           # 将256个中断门全部设成一样的 ignore_interrupt
	ret

/*
 * setup_gdt
 *
 * This routines sets up a new gdt and loads it.
 * Only two entries are currently built, the same
 * ones that were built in init.s. The routine
 * is VERY complicated at two whole lines, so this
 * rather long comment is certainly needed :-)
 * This routine will be overwritten by the page tables.
 */
setup_gdt:
	lgdt gdt_descr
	ret

/*
 * I put the kernel page tables right after the page directory,
 * using 4 of them to span 16 Mb of physical memory. People with
 * more than 16Mb will have to expand this.
 */
.org 0x1000 # 设置起始地址伪指令 ORG：ORG 数值表达式。
            # 作用：指明指令代码或数据存放的偏移位置。
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000

/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isn't
 * on a 64KB border.
 */
tmp_floppy_area:
	.fill 1024,1,0

# 
after_page_tables:
	pushl $0         # These are the parameters to main :-)
	pushl $0         # 压栈main参数
	pushl $0
	pushl $L6
	pushl $main
	jmp setup_paging

L6:                  # 跑到这里就卡死
	jmp L6           # main should never return here, but
	                 # Just in case, we know what happens.

/* This is the default interrupt "handler" */
int_msg:
	.asciz "Unknown Interrupt\n\r"
.align 2
ignore_int:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	pushl $int_msg
	call printk
	popl %eax
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

/*
 * Setup_paging
 *
 * This routine sets up paging by setting the page bit
 * 这段代码通过设置cr0寄存器的page位来打开分页
 * in cr0. The page tables are set up, identity-mapping
 * 分页后映射前16MB内存。分页者假定没有非法地址产生（越界）
 * the first 16MB. The pager assmes that no illegal
 * addresses are produced (ie >4Mb on a 4Mb machine).
 *
 * NOTE! Although all physical memory should be identity
 * 注意！即使所有的物理内存都应该被identity，也只有kernel可以
 * mapped by this routine, only the kernel page functions
 * 直接访问1Mb之外的内存。所有“一般的”方法只使用最低1Mb内存
 * use the > 1Mb address directly. All "normal" functions
 * 或者被映射到其他地方的本地数据空间。
 * use just the lower 1Mb, or the local data space, which
 * will be mapped to some other place - mm keeps track of
 * that.
 *
 * For those with more memory than 16 Mb - tough luck. I've
 * not got it, why should you :-) The source is here. Change
 * it. (Seriously - it shouldn't be too difficult. Mostly
 * change some constants etc. I left it at 16Mb, as my machine
 * even cannot be extended past that (OK, but it was cheap :-)
 * I've tried to show which constants to change by having
 * some kind of marker at them (search for "16Mb"), but I
 * won't guarantee that's all :-( )
 */
.align 2
setup_paging:
	movl $1024*5, %ecx      /* 清零0x0000-0x5000 */
	xorl %eax, %eax
	xorl %edi, %edi         /* pg_dir is at 0x00 ，这里覆盖了0x5000前面的代码 */
	cld;rep;stosl           # CLean Direction flag 
	                        # 这里的分号有些浏览器会解释成注释
							# 但是实际rep stosl是执行的
	                        # 当DF=0的时候，从低地址到高地址 */
	movl $pg0+7, pg_dir     # 这里为什么是+7? 误区：pg0的地址+7
	movl $pg1+7, pg_dir+4   # 实际上pg0的偏移值+7（看上面，pg0:  pg1: ...）
	movl $pg2+7, pg_dir+8   # pg0=0x1000 +7 = 0x1007
	movl $pg3+7, pg_dir+12  # 这里四个页目录，映射到四个页表的头部 
	                        # 分别映射到0x1000 0x2000 0x3000 0x4000
							# 每个页表4byte，那么0x1000的空间可以存储 0x1000/4 = 0x400
							# 也就是1024个页表
							# 每个页表映射物理地址范围为0x1000,也就是0x400*0x1000=4MB
							# 四个页目录加起来就=4M*4=16M
	movl $pg3+4092, %edi    # 这里从pg3往回写，pg3的偏移（0x4000）加上4092（0xffc）
	movl $0xfff007, %eax    /* 16Mb - 4096 + 7 (r/w user, p) */
	std                     # SeT Direction flag
	                        # 当DF=1的时候，从高地址到低地址
1:  stosl                   # Store String  4字节4字节地循环写入
                            /* fill pages backwards - more efficient :-) */
							# 为什么backwards更efficient？
							# 难道是因为：
							# 80386没有缓存，80486的缓存为8K = 0x2000
							# 前面第271行，从0x0增长到了1024*5x4=0x5000
							# 这时候L1缓存了从0x5000-0x3000的内容
							# 如果正向cld，那么又重新从0x1000开始写
							# 这时候L1 cache全部miss
							# 如果是反向std，那么从0x4ffc开始，会命中
							# 0x4ffc-0x3000=大小的缓存,(0x4ffc写入long其实写到了0x4fff，这里命中全部0x2000缓存)
	subl $0x1000, %eax
	jge 1b
	cld
	xorl %eax, %eax         /* pg_dir is at 0x0000 */
	                        # 页目录设置在这里，注意！这里只有四条页目录
							# 请看上面向278-281向pg_dir写的四条
							# pg_dir在这个程序头定义了，也就是说写在了本程序的startup_32
							# 覆盖了程序头！

							# 000000a1 <setup_gdt>:  *可以看到这里还是0x0000a1
							#  setup_gdt:
							#          lgdt gdt_descr
							#        a1:       0f 01 15 b2 54 00 00    lgdtl  0x54b2
							#          ret
							#        a8:       c3                      ret    
							#          ...

							#  00001000 <pg0>:       *pg0直接用.org伪指令定位到0x1000
							#          ...

							#  00002000 <pg1>:       *pg1直接定位到0x2000
							#          ...

							#  00003000 <pg2>:       *pg2直接定位到0x3000
							#          ...

							#  00004000 <pg3>:       ........
							#          ...

							#  00005000 <tmp_floppy_area>: * 这里在pg3后面.org了0x5000,从0x5000开始
							#          ...

							#  00005400 <after_page_tables>:
							#  tmp_floppy_area:
							#          .fill 1024,1,0

							#  # 
							#  after_page_tables:
							#          pushl $0         # These are the parameters to main :-)
							#      5400:       6a 00                   push   $0x0
							#          pushl $0         # 压栈main参数
							#      5402:       6a 00                   push   $0x0
							#          pushl $0
							#      5404:       6a 00                   push   $0x0
							#          pushl $L6
							#      5406:       68 12 54 00 00          push   $0x5412
							#          pushl $main
							#      540b:       68 d3 85 01 00          push   $0x185d3
							#          jmp setup_paging
							#      5410:       eb 3c                   jmp    544e <setup_paging>

							#  00005412 <L6>:

                            # 也就是说，运行到写pg_dir和页表的时候，cs:ip已经跑到0x5000外了，这时候可覆盖
							# 前面的0x0000-0x4fff。
							# 为什么页表要写到0x4ffc而不是0x4fff呢？
							# 因为页表有长度啊，从0x4ffc开始写0x00fff807，四位正好写到0x4fff

	movl %eax, %cr3         /* cr3 - page directory start 设置好了cr3 ,就可以打开分页了*/
	movl %cr0, %eax
	orl  $0x80000000, %eax
	movl %eax, %cr0         /* set paging (PG) bit */
							# 设置cr0寄存器的PG位，打开分页
	ret                     /* This also flushes prefetch-queue */
                            # 这里直接ret返回到main，正式进入内核高级语言代码执行

.align 2
.word 0
idt_descr:                   # 这只是一个描述符
	.word 256*8-1            # IDT contains 256 entries
	.long idt                # 这里指向真正的表偏移
.align 2
.word 0
gdt_descr:                   # 这只是一个描述符，只有一个描述符的大小
	.word 256*8-1            # 0x7ff,界限
	.long gdt                # 
                             # 这里指向真正的表偏移
	.align 8
idt:  
	.fill 256,8,0            # IDT is uninitialized 0x54b8
	                         # 用0填满256个长度为8的表
# gdt在这个位置！
gdt:                         # 已经设计好的gdt表
	.quad 0x0000000000000000  /* NULL descriptor */ # 第一个一定为空
	.quad 0x00c09a0000000fff  /* 16Mb */            
	.quad 0x00c0920000000fff  /* 16Mb */
	.quad 0x0000000000000000  /* TEMPORARY - don't use */
	.fill 252,8,0             /* space for LDT's and TSS's etc */

/*
段基址（Base Address）：0x00000000，表明段的起始地址在内存的0位置。
段界限（Limit）：0x0000ffff，段界限的单位取决于G（Granularity）标志位的设置，此处表示这个段可以访问前1MB的内存（若G=1，则以4KB为单位，实际大小需乘以4KB）。
标志（Flags）：0x00c09a中，9A 代表了段的类型和访问权限，其中：
*/
# a=0x1010
# 2=0x0010
# 差别在可执行位

