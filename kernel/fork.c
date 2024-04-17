/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/mm.h>
#include <errno.h>
#include <asm/system.h>
#include <linux/kernel.h>

extern void write_verify(unsigned long address);

long last_pid = 0;

void verify_area(void *addr, int size)
{
    unsigned long start;

    start = (unsigned long)addr;
    size += start & 0xfff;
    start &= 0xfffff000;
    start += get_base(current->ldt[2]);
    while (size > 0) {
        size -= 4096;
        write_verify(start);
        start += 4096;
    }
}

int find_empty_process(void)
{
    int i;

repeat:
    if ((++last_pid) < 0)
        last_pid = 1;
    for (i = 0; i < NR_TASKS; i++)
        // 从1开始找pid，1 2 3 4 5 ... 在task[]中如果不存在（没连续起来 1 2 4 5）
        // 那么for就会一直在last_pid=3的地方循环下去，找有没有pid为3的
        if (task[i] && task[i]->pid == last_pid)
            goto repeat;
    // 跑到这里的话就是找完了task都没有pid=3的，说明3可以用，这时候last_pid固定了
    for (i = 1; i < NR_TASKS; i++)
        // 从1开始找task[i]为空的下标，如果有就返回。
        if (!task[i])
            return i;

    return -EAGAIN;
}

int copy_mem(int nr, struct task_struct *p)
{
    unsigned long old_data_base, new_data_base, data_limit;
    unsigned long old_code_base, new_code_base, code_limit;

    code_limit = get_limit(0x0f); /* Obtain Code segment from LDT[1] */
    data_limit = get_limit(0x17); /* Obtain Data segment from LDT[2] */
    old_code_base = get_base(current->ldt[1]); /* Current CODE Segment */
    old_data_base = get_base(current->ldt[2]); /* Current DATA Segment */
    if (old_data_base != old_code_base)
        panic("We don't support separate I&D");
    if (data_limit < code_limit)
        panic("Bad data_limit");
    new_data_base = new_code_base = nr * 0x4000000;
    p->start_code = new_code_base;
    set_base(p->ldt[1], new_code_base);
    set_base(p->ldt[2], new_data_base);
    if (copy_page_tables(old_data_base, new_data_base, data_limit)) {
        printk("free_page_tables: from copy_mem\n");
        free_page_tables(new_data_base, data_limit);
        return -ENOMEM;
    }
    return 0;
}

/*
 * Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entriety.
 */
int copy_process(int nr, long ebp, long edi, long esi, long gs, long none, // 这个none是call的返回地址压栈
		long ebx, long ecx, long edx, //这一行和下面一行是system_call压栈的
		long fs, long es, long ds,
		long eip, long cs, long eflags, long esp, long ss) // 用户进程由用户态切换成内核态时，硬件上自动将指定寄存器压入栈
        // 最后eip cs eflags esp ss这几个参数是int指令执行的时候cpu自动压栈的
        // 如果没有执行级别的转换，最后两个esp和ss是不用压栈的，因为可以直接用原来的栈，就不用开新栈了
        // 可是在main函数中，我们不是还在内核代码中吗？
        // 可以看main函数进fork的上一行，执行了move_to_user_mode()，进入了用户态
        // 那么cpu是怎么判断我们在用户态的呢？
        // 
{ 
    struct task_struct *p;
    int i;
    struct file *f;

    // 通过get_free_page找到并返回一个填充了0x0的4K页物理地址
    p = (struct task_struct *) get_free_page();
    if (!p) // 如果返回的是0x0，那么就是没找到
        return -EAGAIN;

    // task[]在<linux/sched.c中定义>，0是init_task.task
    task[nr] = p; // 这里只是存了一个简单的指针地址，其实指向一个4K的页

    /*
     * NOTE!: the following statement now work with gcc 4.3.2 now, and you
     * must compile _THIS_ memory without no -O of gcc.#ifndef GCC4_3
     */
    *p = *current; // 这个current就是前面手写一堆数据构造出来的init的task_struct
    p->state = TASK_UNINTERRUPTIBLE; // 不可中断的睡眠状态，不可被信号打断
    p->pid = last_pid;          // 在这里直接用上一个函数计算的last_pid init来说上一个函数是find_empty_process
    p->father = current->pid;   // init的father应该是自己吧
    p->counter = p->priority;   // 
    p->signal = 0;              //  看到这里，GPT-4 oracle批量建表里面有struct注释
    p->alarm = 0;               //
    p->leader = 0;
    p->utime = p->stime = 0;    //
    p->cutime = p->cstime = 0;  // 
    p->start_time = jiffies;    // 
    p->tss.back_link = 0;
    p->tss.esp0 = PAGE_SIZE + (long) p;
    p->tss.ss0 = 0x10;
    p->tss.eip = eip;
    p->tss.eflags = eflags;
    p->tss.eax = 0;
    p->tss.ecx = ecx;
    p->tss.edx = edx;
    p->tss.ebx = ebx;
    p->tss.esp = esp;
    p->tss.ebp = ebp;
    p->tss.esi = esi;
    p->tss.edi = edi;
    p->tss.es = es & 0xffff;
    p->tss.cs = cs & 0xffff;
    p->tss.ss = ss & 0xffff;
    p->tss.ds = ds & 0xffff;
    p->tss.fs = fs & 0xffff;
    p->tss.gs = gs & 0xffff;
    p->tss.ldt = _LDT(nr); /* selector for LDT */
    p->tss.trace_bitmap = 0x80000000;
    if (last_task_used_math == current)
        __asm__("clts ; fnsave %0" :: "m" (p->tss.i387));
    if (copy_mem(nr, p)) {
        task[nr] = NULL;
        free_page((long)p);
        return -EAGAIN;
    }
    for (i = 0; i < NR_OPEN; i++)
        if ((f = p->filp[i]))
            f->f_count++;
    if (current->pwd)
        current->pwd->i_count++;
    if (current->root)
        current->root->i_count++;
    if (current->executable)
        current->executable->i_count++;

    set_tss_desc(gdt + (nr << 1) + FIRST_TSS_ENTRY, &(p->tss));
    set_ldt_desc(gdt + (nr << 1) + FIRST_LDT_ENTRY, &(p->ldt));
    p->state = TASK_RUNNING;  /* do this last, just in case */
    return last_pid;
}
