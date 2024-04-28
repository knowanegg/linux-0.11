/*
 * linux/kernel/system_call.s
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * system_call.s contains the system-call low-level handing routines.
 * This also contains the timer-interrupt handler, as some of the code is
 * the same. The hd- and floppy-interrupts are also here.
 *
 * NOTE: this code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call. Ordinary interrupts
 * don't handle signal-recognition, as that would clutter them up totally
 * unnecessarilly.
 *
 * Stack layout in 'ret_from_system_call':
 *
 *       0(%esp) - %eax
 *       4(%esp) - %ebx
 *       8(%esp) - %ecx
 *       C(%esp) - %edx
 *      10(%esp) - %fs
 *      14(%esp) - %es
 *      18(%esp) - %ds
 *      1C(%esp) - %eip
 *      20(%esp) - %cs
 *      24(%esp) - %eflags
 *      28(%esp) - %oldesp
 *      2C(%esp) - %oldss
 */

SIG_CHLD	= 17

EAX               = 0x00
EBX               = 0x04
ECX               = 0x08
EDX               = 0x0C
FS                = 0x10
ES                = 0x14
DS                = 0x18
EIP               = 0x1C
CS                = 0x20
EFLAGS            = 0x24
OLDESP            = 0x28
OLDSS             = 0x2C

state      = 0       # these are offsets into the task-struct.   
counter    = 4
priority   = 8
signal     = 12
sigaction  = 16      # Must be 16 (=len of sigaction)
blocked    = (33 * 16)

# offsets within sigaction
sa_handler = 0
sa_mask    = 4
sa_flags   = 8
sa_restorer = 12

nr_system_calls = 200

/*
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 */
.globl system_call, sys_fork, timer_interrupt, sys_execve 
.globl hd_interrupt, floppy_interrupt, parallel_interrupt
.globl device_not_available, coprocessor_error

.align 2
bad_sys_call:
	movl $-1, %eax
	iret
.align 2
reschedule:
        pushl $ret_from_sys_call
        jmp schedule
.align 2
system_call: # 这里就是int跳到的地方了，这里的压栈就是int的压栈，cpu不参与
        cmpl $nr_system_calls-1,%eax	# 检查syscall的nr是否在范围内
        ja bad_sys_call
        push %ds
        push %es
        push %fs
        pushl %edx
        pushl %ecx              # push %ebx,%ecx,%edx as parameters
        pushl %ebx              # 压入顺序 ds es fs edx ecx ebx
        movl $0x10,%edx         # set up ds,es to kernel space
        mov %dx,%ds
        mov %dx,%es
        movl $0x17,%edx         # fs points to local data space
        mov %dx,%fs
        call *sys_call_table(,%eax,4) # 在这里进入调用
        pushl %eax
        movl current,%eax
        cmpl $0,state(%eax)             # state
        jne reschedule
        cmpl $0,counter(%eax)           # counter
        je reschedule
ret_from_sys_call:
	movl current, %eax
	cmpl task, %eax
	je 3f
	cmpw $0x0f, CS(%esp)
	jne 3f
	cmpw $0x17, OLDSS(%esp)
	jne 3f
	movl signal(%eax), %ebx
	movl blocked(%eax), %ecx
	notl %ecx
	andl %ebx, %ecx
	bsfl %ecx, %ecx
	je 3f
	btrl %ecx, %ebx
	movl %ebx, signal(%eax)
	incl %ecx
	pushl %ecx
	call do_signal
	popl %eax
3:	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret

.align 2
coprocessor_error:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	movl $0x17, %eax
	mov %ax, %fs
	pushl $ret_from_sys_call
	jmp math_error

.align 2
device_not_available:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10, %eax
	mov  %ax, %ds
	mov  %ax, %es
	movl $0x17, %eax
	mov  %ax, %fs
	pushl $ret_from_sys_call
	clts
	movl %cr0, %eax
	testl $0x4, %eax
	je math_state_restore
	pushl %ebp
	pushl %esi
	pushl %edi
	call math_emulate
	popl %edi
	popl %esi
	popl %ebp
	ret

.align 2
timer_interrupt:
	push %ds              # save ds, es and put kernel data space
	push %es              # into them. %fs is used by _system_call
	push %fs			  # 保存 ds、es 并让其指向内核数据段。fs 将用于 system_call。
	pushl %edx            # we save %eax, %ecx, %edx as gcc doesn't
	pushl %ecx            # save those across function calls. %ebx
	pushl %ebx            # is saved as we use that in ret_sys_call
	pushl %eax
	movl $0x10, %eax
	mov %ax, %ds
	mov %ax, %es          # 设置ds和es为0x10选择子
	movl $0x17, %eax      # 索引为0x5（因为要移位3位，即0x17 >> 3得到3）。
	mov %ax, %fs		  # RPL为3，表示这是一个用于用户模式的段。TI为1，表示从LDT中取
						  # 但是这不是内核吗？有LDT吗？有！不要忘了内核main在fork前执行了move_to_user_mode
						  # 这里就是取LDT表中第三项，在move_to_user_mode中看设置，是局部数据段（程序的数据段）
	incl jiffies		  # 时钟中断计数器+1
	movb $0x20, %al       # EOI to interrupt controller #1
	outb %al, $0x20      
	movl CS(%esp), %eax   # CS(%esp)表示从堆栈中取出CS寄存器的值？！还有这种操作？cs寄存器什么时候进去的？又因为什么可以直接用CS(%esp)取到？
						  # 还记得call吗？call之前是不是要压栈返回地址？这个返回地址就是CS:IP组成的啊，当然是固定能找到的
	andl $3, %eax         # 从堆栈中取出执行系统调用代码的选择符（CS 段寄存器值）中的当前特权级别(0 或 3)并压入
	pushl %eax            # 堆栈，作为 do_timer 的参数。do_timer()函数执行任务切换、计时等工作，在 kernel/sched.c
	call do_timer         
	addl $4, %esp
	jmp ret_from_sys_call
	ret

.align 2
sys_execve:
	lea EIP(%esp), %eax
	pushl %eax
	call do_execve
	addl $4, %esp
	ret

.align 2
sys_fork: # 进来之前是通过了系统调用，已经压栈了ds es fs edx ecx ebx
	call find_empty_process
	testl %eax, %eax    # 返回eax
	js 1f 				# js （Jump if Sign）是一个条件跳转指令，它会检查符号标志（SF）的状态。
          				# 执行逻辑：如果 SF 被设置（即为1，表示最近的算术或逻辑操作的结果是负数），则跳转到指定的标签
						# find_empty_process 返回 -EAGAIN 是-11, 表示任务数组中已经没空位置了
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax			# 这里的eax就是find_empty_process找到的task[]数组内空闲的下标
	call copy_process	# 进去前压入顺序 gs esi edi ebp eax(nr)
						# 加上syscall的压栈 ds es fs edx ecx ebx gs esi edi ebp eax(nr)
						# cpu自动压栈下一条指令的地址作为ret地址
	addl $20, %esp
1:  ret

hd_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push  %ds
	push  %es
	push  %fs
	movl  $0x10, %eax
	mov   %ax, %ds
	mov   %ax, %es
	movl  $0x17, %eax
	mov   %ax, %fs
	movb  $0x20, %al
	outb  %al, $0xA0     # EOI to interrupt controller #1
	jmp   1f             # give port chance to breathe
1:	jmp   1f
1:	xorl  %edx, %edx
	xchgl do_hd, %edx
	testl %edx, %edx
	jne 1f
	movl $unexpected_hd_interrupt, %edx
1:	outb %al, $0x20
	call *%edx           # "interrupting" way of handing intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

floppy_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push  %ds
	push  %es
	push  %fs
	movl $0x10, %eax
	mov  %ax, %ds
	mov  %ax, %es
	movl $0x17, %eax
	mov  %ax, %fs
	movb $0x20, %al
	outb %al, $0x20         # EOI to interrupt controller #1
	xorl %eax, %eax
	xchgl do_floppy, %eax
	testl %eax, %eax
	jne 1f
	movl $unexpected_floppy_interrupt, %eax
1:	call *%eax              # "interrupting" way of handing intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

parallel_interrupt:
	pushl %eax
	movb $0x20, %al
	outb %al, $0x20
	popl %eax
	iret
