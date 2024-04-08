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
void schedule(void)
{
	int i, next, c;
	struct task_struct **p;

	/*
	 * Check alarm, wake up any interruptible tasks that have
	 * got a signal.
	 */
	for (p = &LAST_TASK; p > &FIRST_TASK; --p) 
		if (*p) {
			if ((*p)->alarm && (*p)->alarm < jiffies) {
				(*p)->signal |= (1 << (SIGALRM - 1));
				(*p)->alarm = 0;
			}
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			    (*p)->state == TASK_INTERRUPTIBLE)
				(*p)->state = TASK_RUNNING;
		}

		/* This is the schedule proper: */
		while (1) {
			c = -1;
			next = 0;
			i = NR_TASKS;
			p = &task[NR_TASKS];

			while (--i) {
				if (!*--p)
					continue;
				if ((*p)->state == TASK_RUNNING &&
				    (*p)->counter > c)
					c = (*p)->counter, next = i;
			}
			if (c)
				break;
			for (p = &LAST_TASK; p > &FIRST_TASK; --p)
				if (*p)
					(*p)->counter = ((*p)->counter >> 1) +
					    (*p)->priority;
		}
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

void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state = 0;
		*p = NULL;
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
    set_system_gate(0x80, &system_call);  // 设置系统调用门0x80
}

#define TIME_REQUESTS 64

static struct timer_list {
	long jiffies;
	void (*fn) (void);
	struct timer_list *next;
} timer_list[TIME_REQUESTS], *next_timer = NULL;

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

void do_timer(long cpl)
{
    extern int beepcount;
    extern void sysbeepstop(void);

    if (beepcount)
        if (!--beepcount)
            sysbeepstop();

    if (cpl)
        current->utime++;
    else
        current->stime++;

    if (next_timer) {
        next_timer->jiffies--;
        while (next_timer && next_timer->jiffies <= 0) {
            void (*fn)(void);

            fn = next_timer->fn;
            next_timer->fn = NULL;
            next_timer = next_timer->next;
            (fn)();
        }
    }
    if (current_DOR & 0xf0)
        do_floppy_timer();
    if ((--current->counter) > 0)
        return;
    current->counter = 0;
    if (!cpl)
        return;
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
