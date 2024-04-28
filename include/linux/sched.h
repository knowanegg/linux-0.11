#ifndef _SCHED_H_
#define _SCHED_H_

#include <linux/head.h>
#include <linux/fs.h>
#include <signal.h>

#define NR_TASKS 64
#define HZ 100

#define FIRST_TASK task[0]
#define LAST_TASK  task[NR_TASKS - 1]

#define TASK_RUNNING         0
#define TASK_INTERRUPTIBLE   1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_ZOMBIE          3
#define TASK_STOPPED         4

#ifndef NULL
#define NULL ((void *) 0)
#endif

typedef int (*fn_ptr)(void);

struct i387_struct {
	long cwd;
	long swd;
	long twd;
	long fip;
	long fcs;
	long foo;
	long fos;
	long st_space[20];
};

// TSS（任务状态段 Task State Segment）
// 用于存储关于任务（通常指进程或线程）的信息。TSS 使得处理器能够保存和恢复任务的状态，从而支持任务之间的切换（例如，通过中断或异常引起的任务切换）。这对于实现多任务操作系统至关重要。
// TSS 的主要内容包括：
// EIP（指令指针）：存储任务被中断时的下一条指令的地址。
// ESP（堆栈指针）和 EBP（基指针）：用于保存任务的堆栈状态。
// 段寄存器：包括 CS（代码段）、DS（数据段）、ES、FS、GS 和 SS（堆栈段）寄存器，它们定义了任务的内存空间布局。
// CR3（页目录基址寄存器）：用于虚拟内存管理，指向任务的页目录。
// 标志寄存器（EFLAGS）：保存处理器的状态标志。
// 通用寄存器：包括 EAX、EBX、ECX、EDX、EDI 和 ESI。
struct tss_struct {
	long back_link;
	long esp0;
	long ss0;
	long esp1;
	long ss1;
	long esp2;
	long ss2;
	long cr3;
	long eip;
	long eflags;
	long eax, ecx, edx, ebx;
	long esp;
	long ebp;
	long esi;
	long edi;
	long es;
	long cs;
	long ss;
	long ds;
	long fs;
	long gs;
	long ldt;
	long trace_bitmap;
	struct i387_struct i387;
};

struct task_struct {
	/* 基本进程状态和调度信息 */
	long state;		/* -1 unrunable, 0 runable, >0 stopped */
	long counter;   // 时间片计数
	long priority;  // 优先级，这里还没上CFS那种算法
	long signal;    // 这个字段是一个指向 signal_struct 类型的指针。
	struct sigaction sigaction[32]; // 针对32种不同信号的处理动作数组。
	long blocked;        // 掩码，标识哪些信号被阻塞。
	/* 进程生命周期和状态信息 */
	int exit_code;  // 进程退出时的状态码。
	unsigned long start_code, end_code, end_data, brk, start_stack; // 段的起始和结束地址。
	long pid, father, pgrp, session, leader; // pgrp：进程组ID。session会话id。leader 会话领导者ID。
	// 用户和组ID信息
	unsigned short uid, euid, suid; // 用户ID、有效用户ID和保存的用户ID。
	unsigned short gid, egid, sgid; // 组ID、有效组ID和保存的组ID。
	long alarm;  // 闹钟信号的剩余时间。
	long utime, stime, cutime, cstime, start_time; // 用户CPU时间、系统CPU时间、累计的子进程用户CPU时间和系统CPU时间。
	unsigned short used_math;  // 进程开始的时间。
	/* 文件系统和I/O信息 */
	int tty;   /* -1 if no tty, so it must be signed */ //关联的终端设备（-1表示无终端）。
	unsigned short umask; // 文件模式创建掩码。
	struct m_inode *pwd; // 当前工作目录的inode。
	struct m_inode *root; // 根目录的inode。
	struct m_inode *executable; // 可执行文件的inode。
	unsigned long close_on_exec; // 执行时关闭文件描述符的位掩码。
	struct file *filp[NR_OPEN]; // 打开文件描述符数组。
	/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3]; // 本地描述符表，包含三个段描述符，通常用于定义代码段（CS）和数据段/堆栈段（DS/SS）。
	/* tss for this task */
	struct tss_struct tss; // 任务状态段，用于任务切换和硬件任务管理。
};

/*
 * INIT_TASK is used to set up the first task table, touch at
 * your own risk! Base=0, limit=0x9ffff (=640kB)
 */
#define INIT_TASK  \
 /* state etc */  { 0, 15, 15,      \
 /* signals */      0, { {},}, 0,   \
 /* ec,brk */       0, 0, 0, 0, 0, 0,  \
 /* pid etc */      0, -1, 0, 0, 0,    \
 /* uid etc */      0, 0, 0, 0, 0, 0,  \
 /* alarm */        0, 0, 0, 0, 0, 0,  \
 /* math */         0,                 \
 /* fs info */      -1, 0022, NULL, NULL, NULL, 0, \
 /* filp */         { NULL,},                      \
 /* LDT */        {                              \
 /* LDT-0 NULL */     {0, 0},                    \
 /* LDT-1 CODE */     {0x9f, 0xc0fa00},          \
 /* LDT-2 DATA */     {0x9f, 0xc0f200},          \
                  },                             \
 /* tss */        { 0, PAGE_SIZE + (long)&init_task, 0x10, 0, 0, \
                    0, 0, (long)&pg_dir,           \
                    0, 0, 0, 0, 0, 0, 0, 0, \
                    0, 0, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,     \
                    _LDT(0), 0x80000000,    \
                    {}                      \
                  }, \
}

/*
 * Entry into gdt where to find first TSS.
 * 0 - nul, 1 - cs, 2 - ds, 3 - syscall
 * 4 - TSS0, 5 - LDT0, 6 - TSS1 etc ...
 */
#define FIRST_TSS_ENTRY  4
#define FIRST_LDT_ENTRY  (FIRST_TSS_ENTRY + 1)
#define _TSS(n)  ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n)  ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
#define ltr(n)   __asm__("ltr %%ax" :: "a" (_TSS(n)))
#define lldt(n)  __asm__("lldt %%ax" :: "a" (_LDT(n)))
#define str(n)   \
        __asm__("str %%ax\n\t"   \
                "subl %2, %%eax\n\t"    \
                "shrl $4, %%eax"        \
                :"=a" (n)   \
                :"a" (0), "i" (FIRST_TSS_ENTRY<<3))

/*
 * switch_to(n) - should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it
 * does nothing.
 * This also clears the TS-flag if the task we switched to
 * has used tha math co-processor latest.
 */
#define switch_to(n) { \
	struct { long a, b;} __tmp;   \
	__asm__("cmpl %%ecx, current\n\t" \
			"je 1f\n\t"    \
			"movw %%dx, %1\n\t"   \
			"xchgl %%ecx, current\n\t"   \
			"ljmp *%0\n\t"       \
			"cmpl %%ecx, last_task_used_math\n\t"   \
			"jne 1f\n\t"      \
			"clts\n"   \
			"1:"  \
			::"m"  (*&__tmp.a), "m" (*&__tmp.b),   \
			"d" (_TSS(n)), "c" ((long) task[n]));  \
}

static inline unsigned long _get_base(char *addr)
{
    unsigned long __base;

    __asm__("movb %3, %%dh\n\t" 
            "movb %2, %%dl\n\t" 
            "shll $16, %%edx\n\t" 
            "movw %1, %%dx":"=&d"(__base)
            : "m"(*((addr) + 2)), "m"(*((addr) + 4)), "m"(*((addr) + 7)));
    return __base;
}

#define _set_base(addr, base)       \
        __asm__("push %%edx\n\t"        \
                "movw %%dx, %0\n\t"     \
                "rorl $16, %%edx\n\t"   \
                "movb %%dl, %1\n\t"     \
                "movb %%dh, %2\n\t"     \
                "pop %%edx"             \
                :: "m" (*((addr) + 2)), \
                "m" (*((addr) + 4)),    \
                "m" (*((addr) + 7)),    \
                "d" (base)              \
)

#define _set_limit(addr, limit)     \
	__asm__("push %%edx\n\t"        \
			"movw %%dx, %0\n\t"     \
			"rorl $16, %%edx\n\t"   \
			"movb %1, %%dh\n\t"     \
			"andb $0xf0, %%dh\n\t"  \
			"orb %%dh, %%dl\n\t"    \
			"movb %%dl, %1\n\t"     \
			"pop %%edx"             \
			:: "m" (*(addr)),       \
			"m" (*((addr) + 6)),    \
			"d" (limit)             \
			)

#define set_base(ldt, base)   _set_base( ((char *)&(ldt)),(base) )
#define set_limit(ldt, limit) _set_limit( ((char *)&(ldt)), (limit - 1) >> 12)

#define get_base(ldt) _get_base((char *)&(ldt))

#define get_limit(segment)  ({    \
	unsigned long __limit;        \
	__asm__ ("lsll %1, %0\n\tincl %0":"=r" (__limit):"r" (segment));  \
	__limit;})

extern struct task_struct *current;
extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *last_task_used_math;
extern long volatile jiffies;
extern void sleep_on(struct task_struct **p);


extern void schedule(void);
extern int tty_write(unsigned minor, char *buf, int count);
extern void interruptible_sleep_on(struct task_struct **);
extern void wake_up(struct task_struct **);
extern void trap_init(void);
extern void sched_init(void);
extern void add_timer(long, void (*fn) (void));
extern int copy_page_tables(unsigned long, unsigned long, long);
extern int free_page_tables(unsigned long, unsigned long);
extern void verify_area(void *addr, int size);

extern long startup_time;
#define CURRENT_TIME (startup_time + jiffies / HZ)

#ifndef PANIC
void panic(const char *str);
#endif
#endif
