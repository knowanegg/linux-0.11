/*
 *  linux/mm/memory.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/kernel.h>
#include <linux/sched.h>
/*
 * These are not to be changed without changing head.s etc.
 * Current version only support litter than 16Mb.
 */


#define LOW_MEM        0x100000 //用于标识系统中可用于普通分配的最低物理内存地址。
#define PAGING_MEMORY  (15 * 1024 * 1024)
#define PAGING_PAGES   (PAGING_MEMORY >> 12)
#define MAP_NR(addr)   (((addr) - LOW_MEM) >> 12)
#define USED 100

#define invalidate() \
    __asm__("movl %%eax, %%cr3" :: "a" (0))

#define copy_page(from, to) \
   __asm__("cld ; rep ; movsl" ::"S" (from), "D" (to), "c" (1024))

static long HIGH_MEMORY = 0;
extern void do_exit(int error_code);

unsigned char mem_map[PAGING_PAGES] = { 0, };

static inline void oom(void)
{
	printk("out of memory\n\r");
	do_exit(SIGSEGV);
}

/*
 * Initialize memory, build mapping array.
 */
void mem_init(long start_mem, long end_mem)
{
    int i;

    HIGH_MEMORY = end_mem;
	// 将字节转换为 4KB 大小的页面数量。
    for (i = 0; i < PAGING_PAGES; i++)
	    // USED = 100
        mem_map[i] = USED;
    // MAP_NR就是=LOW_MEM>>12 快速计算出4kb数量
    i = MAP_NR(start_mem);
	// 0xfe0000-0x40000=0xbe0000
    end_mem -= start_mem;
	// 算算主要内存有多少个页
    end_mem >>= 12;
    while (end_mem-- > 0)
	    // 将所有的主要内存页表填0
        mem_map[i++] = 0;
}

/*
 * Get physical address of first (actully last :-) free page, and mark it
 * used. If no free pages left, return 0.
 */
// 它首先扫描内存页面字节图数组 mem_map[]，寻找值是 0 的字节项（对应空闲页面）。
// 若无则返回 0 结束，表示物理内存已使用完。若找到值为 0 的字节，则将其置 1，并换算出对应空闲页
// 面的起始地址。然后对该内存页面作清零操作，并且最后返回该空闲页面的物理内存起始地址。
unsigned long get_free_page(void)
{
	register unsigned long __res asm("ax");

// => 0x00006573 <+0>:     push   edi
//    0x00006574 <+1>:     call   0x190ec <__x86.get_pc_thunk.ax>
//    0x00006579 <+6>:     add    eax,0x19acb
//    0x0000657e <+11>:    lea    edx,[eax+0x473b]
// <__x86.get_pc_thunk.ax> 函数在 x86 架构的汇编语言编程中主要用于支持生成位置无关代码（Position-Independent Code, PIC）
// 尤其在创建共享库或动态链接库时非常重要。这个函数的作用是将当前的程序计数器（PC）的值加载到 ax 寄存器中。
// 程序计数器是一个寄存器，包含了正在执行的指令的内存地址。
// esp 通常指向调用该 thunk 的指令之后的返回地址。因此，这条指令实际上是在读取调用 __x86.get_pc_thunk.ax 后应当返回的地址，这是调用该函数的下一条指令的地址。
// 然后再add 0x19acb，应该是找到相对于下一条地址偏移0x19acb的位置
// 这里为何是0x19acb,编译器是如何取值的呢？
// 通过gdb跟踪发现，eax加上0x19acb后是0x20044,查看内存,这是<startup_32>的位置，里面放的类似描述符的东西
// pwndbg> x /20x *0x20044
// 0x0 <startup_32>:       0x00001027      0x00002007      0x00003007      0x00004007
// 0x10 <startup_32+16>:   0x00000000      0x00000000      0x00000000      0x00000000
// 0x20 <startup_32+32>:   0x00000000      0x00000000      0x00000000      0x00000000
// startup_32 通常是 Linux 内核启动过程中的一个重要函数，用于 32 位系统的初始化。
// 这里就回到boot三个汇编文件中的head.s
// pg_dir:
//	.globl startup_32
// 下面有setup_paging，就是在这里循环设置了页表，内容为0x1007,0x2007,0x3007....
// 也就是说0x20044是页表开头的位置
// 那么反推，eax（当前pc地址）加上0x19acb后是0x20044是页表开头地址，那么0x19acb就是当前地址pc距离页表开头地址的偏移
// 这个地址编译器是怎么得到的呢？
// 这涉及到编译器在生成位置无关代码（PIC）时的地址解析机制。
// 编译器如何计算偏移值：
// 静态链接阶段的地址计算：
// 在编译和链接程序时，编译器（和链接器）已知所有符号（函数、变量等）的相对位置。
// 即使在生成位置无关代码时，编译器仍然会构建一个完整的符号表，其中包含每个符号相对于某个基点的偏移。
// 这个偏移值是在编译时确定的，基于代码和数据的布局，然后硬编码到最终的机器代码中。
// call __x86.get_pc_thunk.ax
// add $offset, %eax   ; %eax now contains the address of the page table
// 这里$offset是编译器计算出的从当前PC到页表开始位置的偏移。
// 这种方式确保了，无论代码被加载到内存的什么位置，计算出的地址都能正确指向页表。
// offset可以硬编码是因为整个内核二进制文件已经写死了
// 内核中的相对位移也就是offset一定是固定的，而在不同硬件中，内核被加载的位置可能不是固定的

// 这里 "0" (0) 表示将输入寄存器 eax（由于 ax 是 eax 的一部分，因此影响 ax）的值设置为 0。
// 在 GNU 汇编的约定中，数字标签（如 "0"）引用第一个输出操作数
// 这里是 "=a" (__res)。所以 "0" (0) 实际上是将 eax 初始化为 0。
//"i" 是一个约束，用于说明后面的变量（这里是 LOW_MEM）应当被作为立即数（即编译时常量）处理。
//"c" 指示使用 ecx 寄存器来传递变量 PAGING_PAGES 的值。
//"D" 指示使用 edi 寄存器来传递变量 mem_map + PAGING_PAGES - 1 的值。
// mem_map是内存页目录地址，PAGING_PAGES是页目录的数量，-1就是最后一项
//       				   lea    edx, [eax + 0x473b]  //0x473b就是3840个页表-1然后*4长度
//    0x00006584 <+17>:    mov    eax,0x0
//    0x00006589 <+22>:    mov    ecx,0xf00
//    0x0000658e <+27>:    mov    edi,edx
	__asm__("std ; repne ; scasb\n\t"  // scasb = scan stringbtye ,重复检测ax和di位置
									   // 看内存内容的话，字符串一直都是dddddddd(64646464)
									   // 0x24756 <mem_map+3798>: 0x00000000      0x00000000      0x64640000      0x64646464
									   // 0x24766 <mem_map+3814>: 0x64646464      0x64646464      0x64646464      0x64646464
									   // 0x24776 <mem_map+3830>: 0x64646464      0x64646464      0x00006464      0x000000fe
			"jne 1f\n\t"			   // 直到从后往前第一个00， 0x2475e,注意这里地址是数组地址！
			"movb $1, 1(%%edi)\n\t"    // 把1写入找到的位置  0x64640100
			"sall $12, %%ecx\n\t"      // ecx左移12位，也就是*4K，这里ecx应该是循环剩下的计数0xedf,*4K是前面对应的页的总大小
			"addl %2, %%ecx\n\t"       // ecx加上参数2，"i" (LOW_MEM)，0x100000，最低可用物理地址，加上后就是现在设1的页物理地址
			"movl %%ecx, %%edx\n\t"    // 赋值给edx，现在edx成了设1的物理地址，也就是抓到的页 0xfdf000 这个位置是物理地址，free的，可以拿来当一个页存数据
			"movl $1024, %%ecx\n\t"    // ecx设置成立即数1024
			"leal 4092(%%edx), %%edi\n\t"  // 4092+edx -> esi:[edi] ,4092看起来是不是很熟悉？熟悉就错了，不是页表最后一项，看下面
									   // 这里edx+4092，那么就是挪到了当前抓到的页的最后4byte，这里放着的是什么呢?这里面就是页表
			"rep ; stosl\n\t"          // 看这里，为什么是4092，不是为了操作页表最后一位，而是从最后4字节开始往前覆盖0x0
			"movl %%edx, %%eax\n"      // 把edx放进eax中，就是物理地址放进eax，返回的时候就知道free页的地址了
			"1:cld"                    // 没找到的话就清除方向位，直接返回eax，这里是0x0
			: "=a" (__res)
			: "0" (0), "i" (LOW_MEM), "c" (PAGING_PAGES),
			  "D" (mem_map + PAGING_PAGES - 1)
			  );
	return __res;
}

/*
 * Free a page of memory at physical address 'addr'. Used by
 * 'free_page_tables()'
 */
void free_page(unsigned long addr)
{
	if (addr < LOW_MEM)
		return;
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	addr -= LOW_MEM;
	addr >>= 12;
	if (mem_map[addr]--)
		return;
	mem_map[addr] = 0;
	panic("trying to free free page");
}

/*
 * Well, here is one of the most complicated function in mm. It
 * copies a range of linerar address by copying only the pages.:-
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)
 *
 * Note! We don't copy just any chunks of memory - addresses have to
 * be divisible by 4Mb (one page-directory entry), as this makes the
 * function easier. It's used only by fork anyway.
 *
 * Note 2!! When from==0 we are copying kernel space for the first
 * fork(). Then we DONT want to copy a full page-directory entry, as
 * that would lead to some serious memory waste - we just copy the
 * first 160 pages - 640kB. Even that is more than we need, but it
 * doesn't take any more memory - we don't copy-on-write in the low
 * 1 Mb-range, so the pages can be shared with the kernel. Thus the
 * special case for nr=xxxx.
 */
int copy_page_tables(unsigned long from, unsigned long to, long size)
{
    unsigned long *from_page_table;
    unsigned long *to_page_table;
    unsigned long this_page;
    unsigned long *from_dir, *to_dir;
    unsigned long nr;

    /* PDE alignment check: linear address [21:0] must be clear */
    if ((from & 0x3fffff) || (to & 0x3fffff))
        panic("copy_page_tables called with wrong alignment");
    from_dir = (unsigned long *) ((from >> 20) & 0xFFC);  /* _pg_dir = 0 */
    to_dir = (unsigned long *) ((to >> 20) & 0xFFC);
    size = ((unsigned) (size + 0x3fffff)) >> 22;
    for ( ; size-- > 0; from_dir++, to_dir++) {
        /* Check PDE P flag */
        if (1 & *to_dir)
            panic("copy_page_tables: already exist");
        if (!(1 & *from_dir))
            continue;
        from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
        if (!(to_page_table = (unsigned long *) get_free_page()))
            return -1; /* Out of memory, see freeing */
        /* Addition: Set Write/Read, U/S and P flag */
        *to_dir = ((unsigned long) to_page_table) | 7;
        nr = (from == 0) ? 0xA0 : 1024;
        /* Copy old PTE contents to new PTE */
        for ( ; nr-- > 0; from_page_table++, to_page_table++) {
            this_page = *from_page_table;
            if (!(1 & this_page))
                continue;
            /* Only read this 4-KByte page */
            this_page &= ~2;
            *to_page_table = this_page;
            if (this_page > LOW_MEM) {
                *from_page_table = this_page;
                this_page -= LOW_MEM;
                this_page >>= 12;
                mem_map[this_page]++;
            }
        }
    }
    invalidate();
    return 0;
}

/*
 * This function frees a continuos block of page talbes, as needed
 * by 'exit()'. As does copy_page_tables(). This handles only 4Mb blocks.
 */
int free_page_tables(unsigned long from, unsigned long size)
{
    unsigned long *pg_table;
    unsigned long *dir, nr;

    if (from & 0x3fffff)
        panic("free_page_tables called while wrong alignment");
    if (!from)
        panic("Trying to free up swapper memory space");
    size = (size + 0x3fffff) >> 22;
    dir = (unsigned long *) ((from >> 20) & 0xffc); /* _pg_dir = 0 */
    for ( ; size-- > 0; dir++) {
        if (!(1 & *dir))
            continue;
        pg_table = (unsigned long *) (0xfffff000 & *dir);
        for (nr = 0; nr < 1024 ; nr++) {
            if (1 & *pg_table)
                free_page(0xfffff000 & *pg_table);
            *pg_table = 0;
            pg_table++;
        }
        free_page(0xfffff000 & *dir);
        *dir = 0;
    }
    invalidate();
    return 0;
}

/*
 * Free a page of memory at physical address 'addr'. Used by
 * 'free_page_tables()'
 */
void free_table(unsigned long addr)
{
	if (addr < LOW_MEM)
		return;
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	addr -= LOW_MEM;
	addr >>= 12;
	if (mem_map[addr]--)
		return;
	panic("trying to free free page");
}

/*
 * This function puts a page in memory at the wanted address.
 * It returns the physical address of the page gotten, 0 if
 * out of memory (either when trying to access page-table or 
 * page.)
 */
unsigned long put_page(unsigned long page, unsigned long address)
{
	unsigned long tmp, *page_table;

	/* NOTE !!! This uses the fact that _pg_dir=0 */
	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n", page, address);
	if (mem_map[(page - LOW_MEM) >> 12] != 1)
		printk("mem_map disagrees with %p at %p\n", page, address);
	page_table = (unsigned long *)((address >> 20) & 0xffc);
	if ((*page_table) & 1)
		page_table = (unsigned long *)(0xfffff000 & *page_table);
	else {
		if (!(tmp = get_free_page()))	
			return 0;
		*page_table = tmp | 7;
		page_table = (unsigned long *)tmp;
	}
	page_table[(address >> 12) & 0x3ff] = page | 7;
	/* no need for invalidate */
	return page;
}

void get_empty_page(unsigned long address)
{
	unsigned long tmp;

	if (!(tmp = get_free_page()) || !put_page(tmp, address)) {
		free_page(tmp);
		oom();
	}
}

/*
 * try_to_share() checks the page at address "address" in the task "p"
 * to see if it exists, and if it is clean. If so, share it with the current
 * task.
 * 
 * NOTE! This assumes we have checked that p != current, and that they
 * share the same executable.
 */
static int try_to_share(unsigned long address, struct task_struct *p)
{
	unsigned long from;
	unsigned long to;
	unsigned long from_page;
	unsigned long to_page;
	unsigned long phys_addr;

	from_page = to_page = ((address >> 20) & 0xffc);
	from_page += ((p->start_code >> 20) & 0xffc);
	to_page   += ((current->start_code >> 20) & 0xffc);
	/* is there a page-directory at from */
	from = *(unsigned long *) from_page;
	if (!(from & 1))
		return 0;
	from &= 0xfffff000;
	from_page = from + ((address >> 10) & 0xffc);
	phys_addr = *(unsigned long *) from_page;
	/* is the page clean and present? */
	if ((phys_addr & 0x41) != 0x01)
		return 0;
	phys_addr &= 0xfffff000;
	if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
		return 0;
	to = *(unsigned long *) to_page;
	if (!(to & 1)) {
		if ((to = get_free_page()))
			*(unsigned long *) to_page = to | 7;
		else
			oom();
	}
	to &= 0xfffff000;
	to_page = to + ((address >> 10) & 0xffc);
	if (1 & *(unsigned long *) to_page)
		panic("try_to_shar`e: to_page already exists");
	/* share them: write-protect */
	*(unsigned long *) from_page &= ~2;
	*(unsigned long *) to_page = *(unsigned long *) from_page;
	invalidate();
	phys_addr -= LOW_MEM;
	phys_addr >>= 12;
	mem_map[phys_addr]++;
	return 1;
}

/*
 * share_page() tries to find a process that could share a page with
 * the current one. Address is the address of that wanted page relative
 * to the current data space.
 * 
 * We first check if it at all feasible by checking executable->i_count.
 * It should by >1 if there are other tasks sharing this inode.
 */
static int share_page(unsigned long address)
{
	struct task_struct **p;

	if (!current->executable)
		return 0;
	if (current->executable->i_count < 2)
		return 0;
	for (p = &LAST_TASK; p > &FIRST_TASK; --p) {
		if (!*p)
			continue;
		if (current == *p)
			continue;
		if ((*p)->executable != current->executable)
			continue;
		if (try_to_share(address, *p))
			return 1;
	}
	return 0;
}

void do_no_page(unsigned long error_code, unsigned long address)
{
	int nr[4];
	unsigned long tmp;
	unsigned long page;
	int block, i;

	address &= 0xfffff000;
	tmp = address - current->start_code;
	if (!current->executable || tmp >= current->end_data) {
		get_empty_page(address);
		return;
	}
	if (share_page(tmp))
		return;
	if (!(page = get_free_page()))
		oom();
	/* remeber that 1 block is used for header */
	block = 1 + tmp / BLOCK_SIZE;
	for (i = 0; i < 4; block++, i++)
		nr[i] = bmap(current->executable, block);
	bread_page(page, current->executable->i_dev, nr);
	i = tmp + 4096 - current->end_data;
	tmp = page + 4096;
	while (i-- > 0) {
		tmp--;
		*(char *)tmp = 0;
	}
	if (put_page(page, address))
		return;
	free_page(page);
	oom();
}

void un_wp_page(unsigned long *table_entry)
{
	unsigned long old_page, new_page;

	old_page = 0xfffff000 & *table_entry;
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)] == 1) {
		*table_entry |= 2;
		invalidate();
		return;
	}
	if (!(new_page = get_free_page()))
		oom();
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	*table_entry = new_page | 7;
	invalidate();
	copy_page(old_page, new_page);
}

/*
 * This function handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * If it't in code space we exit with a segment error.
 */
void do_wp_page(unsigned long error_code, unsigned long address)
{
#if 0
	/* we cannot do this yet: the estdio library writes to
	* code space stupid, stupid. I really want the libc.a from GNU */
	if (CODE_SPACE(address))
		do_exit(SIGSEGV);
#endif
	un_wp_page((unsigned long *)
		(((address >> 10) & 0xffc) + (0xfffff000 &
		*((unsigned long *) ((address >> 20) & 0xffc)))));
}

void calc_mem(void)
{
	int i, j, k, free = 0;
	long *pg_tbl;

	for (i = 0; i < PAGING_PAGES ; i++)
		if (!mem_map[i])
			free++;
	printk("%d pages free (of %d)\n\r", free, PAGING_PAGES);
	for (i = 2; i < 1024; i++) {
		if (1 & pg_dir[i]) {
			pg_tbl = (long *)(0xfffff000 & pg_dir[i]);
			for (j = k = 0; j < 1024; j++)
				if (pg_tbl[j] & 1)
					k++;
				printk("Pg-dir[%d] uses %d pages\n", i, k);
		}	
	}
}

void write_verify(unsigned long address)
{
    unsigned long page;

    if (!((page = *((unsigned long *)((address >> 20) & 0xffc))) & 1))
        return;
    page &= 0xfffff000;
    page += ((address >> 10) & 0xffc);
    if ((3 & *(unsigned long *) page) == 1) /* non-writeable, present */
        un_wp_page((unsigned long *)page);
    return;
}
