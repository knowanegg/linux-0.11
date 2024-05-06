/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/sys.h>

#include <asm/system.h>
#include <asm/io.h>

#include <linux/fdreg.h>

#define _S(nr)  (1 << ((nr)-1))
// 可以被BLOCK,判断依据是SIGKILL和SIGSTOP位
// 结果等于11111111111101111111111011111111
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

#define LATCH (1193180/HZ)

long user_stack[PAGE_SIZE >> 2];

struct {
	long *a;
	short b;
} stack_start = {
&user_stack[PAGE_SIZE >> 2], 0x10};

union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

static union task_union init_task = { INIT_TASK, };

// jiffies 是系统从开机开始算起的滴答数（10ms)
long volatile jiffies = 0;
long startup_time = 0;
struct task_struct *current = &(init_task.task);
struct task_struct *last_task_used_math = NULL;

struct task_struct *task[NR_TASKS] = { &(init_task.task), };

extern int timer_interrupt(void);
extern int system_call(void);

/*
 * schedule() is the scheduler function. This is GOOD CODE!
 * There probably won't be any reason to change this, as it should
 * work well in all circumstances (ie gives IO-bound processes good
 * response etc).
 * The one thing you might take a look at is the signal-handler
 * here.
 * NOTE! Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep.
 * The information in task[0] is never used.
 */
// 调度函数 schedule()负责选择系统中下一个要运行的任务（进程）。它首先对所有任务进行检测，唤
// 醒任何一个已经得到信号的任务。具体方法是针对任务数组中的每个任务，检查其报警定时值 alarm。
// 如果任务的 alarm 时间已经过期(alarm<jiffies)，则在它的信号位图中设置 SIGALRM 信号，然后清 alarm
// 值。jiffies 是系统从开机计算起的滴答数（10ms/滴答，在 sched.h 中定义）。如果进程的信号位图中除
// 去被阻塞的信号外还有其他信号，并且任务处于可中断睡眠状态（TASK_INTERRUPTIBLE），则置任
// 务为就绪状态（TASK_RUNNING）。
// 随后是调度函数的核心处理部分。这部分代码根据进程的时间片和优先权调度机制，来选择随后要
// 执行的任务。它首先循环检查任务数组中的所有任务，根据每个就绪态任务剩余执行时间的值 counter，
// 选取该值最大的一个任务，并利用 switch_to()函数切换到该任务。
// 若所有就绪态任务的 counter 值都等于零，表示此刻所有任务的时间片都已经运行完，于是就根据任
// 务的优先权值 priority，重置每个任务的运行时间片值 counter，再重新循环检查所有任务的执行时间片值。
void schedule(void)
{
	int i, next, c;
	struct task_struct **p;

	/*
	 * Check alarm, wake up any interruptible tasks that have
	 * got a signal.
	 */
	 // p从task[]数组最后一个开始遍历，第一次init执行时是这样的
	 // task[] = {0x201c0 <init_task>, 0xfdf000, 0x0 <startup_32> <repeats 62 times>}
	 // 也就是0x0有62个，这个循环会循环62次直到0xfdf000才找到一个存在的进程
	for (p = &LAST_TASK; p > &FIRST_TASK; --p) 
		// 如果p存在
		if (*p) {
			// 如果p的闹钟剩余时间alarm还在，且小于系统经过的时钟中断次数
			// 那不对啊，如果这样说，alarm永远赶不上系统的jiffies，因为系统的jiffies是从开机就一直增加的
			// jiffies的定义是准确的，这里只需要理解alarm是如何定义的
			// alarm并不是增长去赶超jiffies的，相反，alarm定义在未来的时间点，等着jiffies到达 
			if ((*p)->alarm && (*p)->alarm < jiffies) { //这里的alarm<jiffies说明已经到时间了，闹钟响了
				(*p)->signal |= (1 << (SIGALRM - 1));   // SIGALRM=0xe，1110，-1=1101，左移1位是11010？错！
														// 这里1<<SIG不是将SIG左移1位，而是将1左移SIG位！
														// 至于为什么-1呢？因为可以不移动啊，如果是0001，非得左移
														// 那么最少就是0010了，最后一位一直空着浪费空间
				(*p)->alarm = 0;	// 闹钟响过了，清零
			}
			// 这里_BLOCKABLE是第19位和9位为0其余为1的SIGNAL位图，这两位对应SIGKILL和SIGSTOP
			// 和(*p)->blocked进行&运算，那么_BLOCKABLE为0的两位会清掉(*p)->blocked对应的2位
			// 意思就是：SIGKILL和SIGSTOP这两位，你想blocked也不行，不允许
			// 然后取反，得到的就是未block的为1的位图，和p的signal相&，就是p进程未被block的信号位图
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
				// 而且要判断p的状态是否为TASK_INTERRUPTIBLE，如果是，可以通过信号、任务超时等手段唤醒
			    (*p)->state == TASK_INTERRUPTIBLE)
				// 上面两个条件是（任务有信号）&&（任务可以被信号唤醒）
				// 那么就吧任务设置为可以执行加入队列
				(*p)->state = TASK_RUNNING;
		}

		/* This is the schedule proper: */
		while (1) {
			c = -1;
			next = 0;
			i = NR_TASKS;
			p = &task[NR_TASKS];
			// 从最后一个TASK开始遍历
			while (--i) {
				// 为空就跳过
				if (!*--p)
					continue;
				// 找到了一个存在的，如果是RUNNING且拥有的剩余时间片比c要大
				if ((*p)->state == TASK_RUNNING &&
				    (*p)->counter > c)
					// 那么c就换为当前人物的时间片，next设置为当前任务下标
					c = (*p)->counter, next = i;
					// 因为在while中，最后肯定会循环到task[0]。
					// 其实就是冒泡排序法找出TASK_RUNNING且counter最大的p
					// 因为用的是>c比较，所以两个进程counter相同时，先执行排名靠后的
			}
			// c大于0，可以执行，退出查找循环，此时next就是这个进程下标
			if (c)
				break;
			// 这里有个情况容易误解，上面过滤的TASK_RUNNING很重要，如果所有的RUNNING进程counter都<=0,
			// 但是存在不是RUNNING的进程counter大于0，也会运行到下面的重新分配
			// 为何这样设计呢？难道是要做到阻塞的时间越长的进程，后续优先级越高吗？
			// 应该是这样的

			// 在所有RUNNING的进程时间片耗尽后，调度器会根据进程的priority字段来重新设置counter的值，以便在下次调度时重新分配时间片。
			// 一般不容易到这个断点，可以用gdb设置值
			// pwndbg> p task
			// $6 = {0x201c0 <init_task>, 0xfdf000, 0xfdd000, 0xfa1000, 0x0 <startup_32> <repeats 60 times>}
			// pwndbg> p (*task[3])->counter=0
			// $7 = 0
			// pwndbg> p (*task[2])->counter=0
			// $8 = 0
			// pwndbg> p (*task[1])->counter=0
			// 或者用 set var width=47，var表示是内核变量而不是gdb的变量
			for (p = &LAST_TASK; p > &FIRST_TASK; --p)
				if (*p) // 再次遍历进程数组
									// 如果，假设不是0，那么这里就是将原有时间片减半的意思
									// 第一次执行这个for循环时，如果某进程为阻塞状态，那么其counter为priority
									// 第二次执行这个for循环时，如果该进程为阻塞状态，那么其counter为priority+priority/2
									// 如果该进程一直阻塞，那么其counter为 p+p/2+p/4+p/8，这样counter一直增加，当该进程一旦处于就绪
									// 状态，它的counter就会变得很大。进程为什么会阻塞，很大可能就是进程IO操作
									// IO操作不正是前台进程的特征吗？也就是说会优先执行前台进程。
					(*p)->counter = ((*p)->counter >> 1) +
					    (*p)->priority;
		}
		// 切换到task[next]执行
		switch_to(next);
}

void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;

	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
repeat: current->state = TASK_UNINTERRUPTIBLE;
	schedule();
        if (*p && *p != current) {
            (**p).state = 0;
            goto repeat;
        }
        *p = NULL;
	if (tmp)
		tmp->state = 0;
}

// 唤醒进程
void wake_up(struct task_struct **p)
{
	// 这里为什么传进来是**p，却要p和*p都不为空？ 
	//  p->*p->**p
	// 这是一个防御性编程的常见做法，确保传入的指针 p 有效，且 p 指向的指针也非空。这样可以防止解引用空指针，导致内核崩溃。
	// debug观察
		// pwndbg> p p  #这一个p是进程指针
		// $15 = (struct task_struct **) 0x27d68
		// pwndbg> p *p
		// $16 = (struct task_struct *) 0xfdf000
		// pwndbg> p **p
		// $17 = {
		//   state = 2,
		//   counter = 15,
		//   priority = 15,
		//   signal = 0,
		//   sigaction = {{
	    //   .....
	// 其实到了**p才是真的数据位置
	// 直接看汇编
	//                         [esp+0x4] 0x00027d68
	//                         ........
	//    0x0000f644 <+17>:    mov    eax,DWORD PTR [esp+0x4] // 找压栈中的值0x27d68所指向de的内存地址，是
	//    						pwndbg>  x/20x (void *) 0x00027d68 // 用gdb找过去，要加void*
	//							0x27d68:        0x00fdf000    // 值是0x00fdf000
    //	  0x0000f648 <+21>:    mov    eax,DWORD PTR [eax]     // 再去找0x00fdf000处内存位置
	// 

	if (p && *p) {
		(**p).state = 0; // 将任务的状态设置为0=runnable, 就绪
		*p = NULL;  // 传进来的时候基本都是 &p ，也就是指针本身的内存位置，*p就是指针指向的地址。设置为空是将这个指针置空
		            // 也就是传进来的这个指针不再有效了。这样做好吗？
					// debug : 0x2110c地址内容 -> 0x00027d68 还在，是&p，而0x00027d68地址内容被清空了,0x00fdf000变成了0x0
	}
}

// sched初始化
void sched_init(void)
{
    int i;
    struct desc_struct *p; //描述符结构体，linus真能复用啊，只要是个32位的都用这个

    if (sizeof(struct sigaction) != 16)
        panic("Struct sigaction MUST be 16 bytes");
	// #define FIRST_TSS_ENTRY  4
	// #define FIRST_LDT_ENTRY  (FIRST_TSS_ENTRY + 1)
	// 也就是tss描述符在gdt中是第四个，FIRST_LDT_ENTRY比ldt多一个
	// 问题：这里的gdt + FIRST_TSS_ENTRY直观解释是gdt+4
	// 那么是gdt地址+4个字节？还是gdt数组往后+4个元素？
	// 实际用gdb观察，是往后跳了四个元素
	// 当你对数组名称执行加法操作时，你实际上是在进行指针运算。数组名称可以被视为指向数组第一个元素的指针。
	// 因此，数组名 + 4 实质上是指向数组开头的指针向后移动了4个元素的位置。
    set_tss_desc(gdt + FIRST_TSS_ENTRY, &(init_task.task.tss)); // 这里重点看tss的定义
    set_ldt_desc(gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));
    p = gdt + 2 + FIRST_TSS_ENTRY; //P在FIRST_TSS_ENTRY往后两个，也就是FIRST_LDT_ENTRY后面

	// 把task置空，p = gdt + 2 + FIRST_TSS_ENTRY后面所有置空
    for (i = 1; i < NR_TASKS; i++) { /* debug n 370 */
        task[i] = NULL;
        p->a = p->b = 0;
        p++;
        p->a = p->b = 0;
        p++;
    }
    /* Clear NT, so that we won't have trouble with that later on */
	// EFLAGS 中的 NT 标志位用于控制任务的嵌套调用。当 NT 位置位时，那么当前中断任务执行
    // IRET 指令时就会引起任务切换。NT 指出 TSS 中的 back_link 字段是否有效。NT=0 时无效。

	// 这里为什么用esp？怎么知道esp对应eflag？
	// 看最前面的pushfl，先压进去操作完再pop出来的..
    __asm__("pushfl ; andl $0xffffbfff, (%esp) ; popfl");

    ltr(0);
    lldt(0);
	// 下面代码用于初始化 8253 定时器。通道 0，选择工作方式 3，二进制计数方式。通道 0 的
    // 输出引脚接在中断控制主芯片的 IRQ0 上，它每 10 毫秒发出一个 IRQ0 请求。LATCH 是初始
 	// 定时计数值
    outb_p(0x36, 0x43);	/* binary, mode 3, LSB/MSB, ch 0 */
    outb_p(LATCH & 0xff, 0x40);	/* LSB */ // 定时值低字节
    outb(LATCH >> 8, 0x40);	/* MSB */ // 定时值高字节。
	//设置0x20中断门为timer_interrupt，timer_interrupt在system_calls.s中用汇编实现
    set_intr_gate(0x20, &timer_interrupt); 
    outb(inb_p(0x21) & ~0x01, 0x21); //// 修改屏蔽码，允许定时器中断。
    set_system_gate(0x80, &system_call);  // 设置系统调用门0x80 后面int 0x80时cpu会去idt表里面找(head.s:106)
}

#define TIME_REQUESTS 64

// 计时器列表，看起来是单向的，只指向next
static struct timer_list {
	long jiffies;
	void (*fn) (void);
	struct timer_list *next;
} timer_list[TIME_REQUESTS], *next_timer = NULL; //next_timer是指向系统下一个要处理的计时器

void add_timer(long jiffies, void (*fn) (void))
{
	struct timer_list *p;

	if (!fn)
		return;
	cli();
	if (jiffies <= 0)
		(fn) ();
	else {
		for (p = timer_list; p < timer_list + TIME_REQUESTS; p++)
			if (!p->fn)
				break;
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time request free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;

		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}

void sleep_on(struct task_struct **p)
{
    struct task_struct *tmp;

    if (!p)
        return;

    if (current == &(init_task.task))
        panic("task[0] trying to sleep");
    tmp = *p;
    *p = current;
    current->state = TASK_UNINTERRUPTIBLE;
    schedule();
    if (tmp)
        tmp->state = 0;
}

/*
 * OK, here are some floppy things that should't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
static struct task_struct *wait_motor[4] = { NULL, NULL, NULL, NULL };
static int mon_timer[4] = { 0, 0, 0, 0 };
static int moff_timer[4] = { 0, 0, 0, 0 };

 // 当前数字输出寄存器 DOR（Digital Output Register）
 // 该变量含有软驱操作中的重要标志，包括选择软驱、控制电机启动、启动复位软盘控制器以
 // 及允许/禁止 DMA 和中断请求。
unsigned char current_DOR = 0x0C;

int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr > 3)
		panic("floppy_on: nr>3");
	moff_timer[nr] = 10000;	/* 100s = very big :-) */
	cli();			/* use floppy_off to turn it off */
	mask |= current_DOR;
	if (!selected) {
		mask &= 0xFC;
		mask |= nr;
	}
	if (mask != current_DOR) {
		outb(mask, FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)
			mon_timer[nr] = HZ / 2;
		else if (mon_timer[nr] < 2)
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();
	return mon_timer[nr];
}

void floppy_on(unsigned int nr)
{
	cli();
	while (ticks_to_floppy_on(nr))
		sleep_on(nr + wait_motor);
	sti();
}

void floppy_off(unsigned int nr)
{
	moff_timer[nr] = 3 * HZ;
}

/*
 * 'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task.
 */
void math_state_restore(void)
{
	if (last_task_used_math == current)
		return;
	__asm__("fwait");
	if (last_task_used_math) {
		__asm__("fnsave %0" :: "m" (last_task_used_math->tss.i387));
	}
	last_task_used_math = current;
	if (current->used_math) {
		__asm__("frstor %0" :: "m" (current->tss.i387));
	} else {
		__asm__("fninit" ::);
		current->used_math = 1;
	}
}

// 针对系统中具有的 4 个软驱，逐一检查使用中的软驱。如果不是 DOR 指定的马达，则跳过
void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i = 0; i < 4; i++, mask <<= 1) {
		if (!(mask & current_DOR))
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])
				wake_up(i + wait_motor);
		} else if (!moff_timer[i]) {
			current_DOR &= ~mask;
			outb(current_DOR, FD_DOR);
		} else
			moff_timer[i]--;
	}
}

 // 定时器中断 C 函数处理程序，在 sys_call.s 中的_timer_interrupt（189 行）中被调用。
 // 参数 cpl 是当前特权级 0 或 3，它是时钟中断发生时正被执行的代码选择符中的特权级。
 // cpl=0 时表示中断发生时正在执行内核代码；cpl=3 时表示中断发生时正在执行用户代码。
 // 对于一个任务，若其执行时间片用完，则进行任务切换。同时函数执行一个计时更新工作。
void do_timer(long cpl)
{
    extern int beepcount; // 在 drivers/chr_drv/console.c中赋值为0
    extern void sysbeepstop(void); //也在console.c中，实现很简单就是outb(inb_p(0x61) & 0xFC, 0x61);

	// 如过没beep完就循环beep
    if (beepcount)
        if (!--beepcount)
            sysbeepstop();

	// 如果cpl>0，那么权限级别比较低，用户级别，就utime++
    if (cpl)
        current->utime++;
	// 否则就stime++
    else
        current->stime++;
	// 这里的utime和stime包含在进程信息里，标志着进程的用时

	// 看看系统有没有下一个要处理的计时器
    if (next_timer) {
        next_timer->jiffies--; // 如果有的话, next_timer的jiffies--，当计时器的jiffies值减至零时，表示计时器已经到期。
        while (next_timer && next_timer->jiffies <= 0) { // 减到0的情况
            void (*fn)(void); // fn是函数指针

            fn = next_timer->fn; // 先保存计时器的处理函数fn
            next_timer->fn = NULL; // 将计时器的fn函数设为NULL
            next_timer = next_timer->next; // 找下下一个计时器
            (fn)(); // 执行上面保存的处理函数
        }
    }

	// 看看是不是软驱
    if (current_DOR & 0xf0)
        do_floppy_timer();
	// 如果当前的counter还没降到0
    if ((--current->counter) > 0)
	// 返回
        return;
	// 如果是0或者0以下，置为0
    current->counter = 0;
	// 如果特权级为0，也就是高权限
    if (!cpl)
	// 返回
        return;
	// 前面条件都不满足，就执行schedule
    schedule();
}

void show_task(int nr, struct task_struct *p)
{
    int i, j = 4096 - sizeof(struct task_struct);

    printk("%d: pid=%d, state=%d, ", nr, p->pid, p->state);
    i = 0;
    while (i < j && !((char *)(p+1))[i])
        i++;
    printk("%d (of %d) chars free in kernel stack\n\r", i, j);
}

void show_stat(void)
{
    int i;

    for (i = 0; i < NR_TASKS; i++)
        if (task[i])
            show_task(i, task[i]);
}

int sys_getpid(void)
{
    return current->pid;
}

int sys_getuid(void)
{
    return current->uid;
}

int sys_alarm(long seconds)
{
    int old = current->alarm;

    if (old)
        old = (old - jiffies) / HZ;
    current->alarm = (seconds > 0) ? (jiffies + HZ * seconds) : 0;
    return old;
}

int sys_pause(void)
{
    current->state = TASK_INTERRUPTIBLE;
    schedule();
    return 0;
}

int sys_nice(long increment)
{
    if (current->priority - increment > 0)
        current->priority -= increment;
    return 0;
}

int sys_getgid(void)
{
    return current->gid;
}

int sys_geteuid(void)
{
    return current->euid;
}

int sys_getegid(void)
{
    return current->egid;
}

int sys_getppid(void)
{
    return current->father;
}
