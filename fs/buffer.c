/*
 * linux/fs/buffer.c
 *
 * (C) 1991 Linus Torvalds
 */
#include <string.h>
#include <stdarg.h>

#include <linux/fs.h>
#include <sys/types.h>
#include <linux/list.h>
#include <asm/system.h>
#include <linux/sched.h>
#include <linux/kernel.h>

// 变量 end 是由链接程序 ld 在链接内核模块时生成，用于指明内核执行模块的末端位置
// 我们可以从编译内核时生成的 System.map 文件中查出该值。这里用它来表明
// 高速缓冲区开始于内核代码末端位置。
extern int end;
extern void put_super(int);

/* 640KB-1MB is used by BIOS and Vedio, it is hard coded, sorry */
#define COPYBLK(from, to) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"movsl\n\t" \
	: : "c" (BLOCK_SIZE/4), "S" (from), "D" (to) \
	)

int NR_BUFFERS = 0;
static struct task_struct *buffer_wait = NULL;
static struct buffer_head *free_list;
struct buffer_head *free_list_head;

struct buffer_head *start_buffer = (struct buffer_head *)&end;
struct buffer_head *hash_table[NR_HASH];

#define _hashfn(dev, block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev, block) hash_table[_hashfn(dev, block)]

static inline void wait_on_buffer(struct buffer_head *bh)
{
    cli();
    while (bh->b_lock)
        sleep_on(&bh->b_wait);
    sti();
}

void brelse(struct buffer_head *bh)
{
    if (!bh)
        return;

    wait_on_buffer(bh);
    if (!(bh->b_count--))
        panic("Trying to free freed buffer.");
    wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It return NULL if the block was unreadable.
 */
struct buffer_head *bread(int dev, int block)
{
    struct buffer_head *bh;

    if (!(bh = getblk(dev, block)))
        panic("Can't get a valid buffer.");

    if (bh->b_uptodate)
        return bh;

    ll_rw_block(READ, bh);
    wait_on_buffer(bh);
    if (bh->b_uptodate)
        return bh;

    brelse(bh);
    return NULL;
}

/* In BiscuitOS, buffer_end is at 4MB currently */
// buffer_end = 0x400000
void buffer_init(long buffer_end)
{
    // start_buffer是end，内核结束的位置
    struct buffer_head *h = start_buffer;
    void *b;
    int i;

    // 0x100000
    if (buffer_end == 1 << 20)
        //  0xa0000
        b = (void *)(640 * 1024);
        // 首先根据参数提供的缓冲区高端位置确定实际缓冲区高端位置 b。如果缓冲区高端等于 1Mb，
        // 则因为从 640KB - 1MB 被显示内存和 BIOS 占用，所以实际可用缓冲区内存高端位置应该是
        // 640KB。否则缓冲区内存高端一定大于 1MB。
        // 在main.c中memory_detect函数如果内存特别小，那么最低限制是从1M开始的主内存
        // 那么buffer_end就是1M
        // 图示：
        //      |  0x00000                      |
        //      |  kernel area                  |  
        //      |  kernel_end =  buffer_start   | kernel结束处就是buffer开始处
        //      |  buffer area                  |
        //      |  640KB   buffer_end_new       |
        //      |  BIOS_VGA                     |
        //      |  1MB     buffer_end_old       | 因为BIOS和VGA用了640KB-1M，buffer_end只能挪上去
        // 这时候buffer只有可怜的640KB，还得减去内核用的空间
    else
        // 如果不止1M，那么就直接设置为buffer_end也就是memory_detect中设置的4M
        b = (void *)buffer_end;
    // BLOCK_SIZE在fs.h中定义，为1024，一个块的大小,0x400,1KB
    // 在这里,b（buffer存储块）向前，h（buffer头）向后，一一对应直到接近
    while ((b -= BLOCK_SIZE) >= ((void *)(h + 1))) {
        h->b_dev    = 0;
        h->b_dirt   = 0;
        h->b_count  = 0;
        h->b_lock   = 0;
        h->b_uptodate = 0;
        h->b_wait = NULL;
        h->b_next = NULL;
        h->b_prev = NULL;
        h->b_data = (char *)b; // 指向b
        h->b_prev_free = h - 1; // 因为都是空闲的
        h->b_next_free = h + 1;
        h++;
        NR_BUFFERS++;
        if (b == (void *)0x100000) // 这里对应1M
            b = (void *)0xA0000;
    }
    h--;
    free_list = start_buffer;
    free_list->b_prev_free = h;
    h->b_next_free = free_list; // 最后一条循环链接到第一条
    // free_list_head 在system.map里有，但是不知道在哪定义的，也没见使用...
    free_list_head = free_list;
    // #define NR_HASH 307
    for (i = 0; i < NR_HASH; i++)
        // 清空哈希表
        hash_table[i] = NULL;
}

int sync_dev(int dev)
{
    int i;
    struct buffer_head *bh;

    bh = start_buffer;
    for (i = 0; i < NR_BUFFERS; i++, bh++) {
        if (bh->b_dev != dev)
            continue;
        wait_on_buffer(bh);
        if (bh->b_dev == dev && bh->b_dirt)
            ll_rw_block(WRITE, bh);
    }
    sync_inodes();
    bh = start_buffer;
    for (i = 0; i < NR_BUFFERS; i++, bh++) {
        if (bh->b_dev != dev)
            continue;
        wait_on_buffer(bh);
        if (bh->b_dev == dev && bh->b_dirt)
            ll_rw_block(WRITE, bh);
    }
    return 0;
}

static struct buffer_head *find_buffer(int dev, int block)
{
    struct buffer_head *tmp;

    for (tmp = hash(dev, block); tmp != NULL; tmp = tmp->b_next)
        if (tmp->b_dev == dev && tmp->b_blocknr == block)
            return tmp;
    return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
struct buffer_head *get_hash_table(int dev, int block)
{
    struct buffer_head *bh;

    for (;;) {
        if (!(bh = find_buffer(dev, block)))
            return NULL;
        bh->b_count++;
        wait_on_buffer(bh);
        if (bh->b_dev == dev && bh->b_blocknr == block)
            return bh;
        bh->b_count--;
    }
}

static inline void insert_into_queues(struct buffer_head *bh)
{
    /* Put at end of free list */
    bh->b_next_free = free_list;
    bh->b_prev_free = free_list->b_prev_free;
    free_list->b_prev_free->b_next_free = bh;
    free_list->b_prev_free = bh;
    /* put the buffer in new hash-queue if it has a device */
    bh->b_prev = NULL;
    bh->b_next = NULL;
    if (!bh->b_dev)
        return;
    bh->b_next = hash(bh->b_dev, bh->b_blocknr);
    hash(bh->b_dev, bh->b_blocknr) = bh;
    bh->b_next->b_prev = bh;
}

static inline void remove_from_queues(struct buffer_head *bh)
{
    /* remove from hash-queue */
    if (bh->b_next)
        bh->b_next->b_prev = bh->b_prev;
    if (bh->b_prev)
        bh->b_prev->b_next = bh->b_next;
    if (hash(bh->b_dev, bh->b_blocknr) == bh)
        hash(bh->b_dev, bh->b_blocknr) = bh->b_next;
    /* remove from free list */
    if (!(bh->b_prev_free) || !(bh->b_next_free))
        panic("Free block list corrupted");
    bh->b_prev_free->b_next_free = bh->b_next_free;
    bh->b_next_free->b_prev_free = bh->b_prev_free;
    if (free_list == bh)
        free_list = bh->b_next_free;
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algorithm is changed: hopefully better, and an elusive bug removed.
 */
#define BADNESS(bh)  (((bh)->b_dirt<<1)+(bh)->b_lock)
struct buffer_head *getblk(int dev, int block)
{
    struct buffer_head *tmp, *bh;

repeat:
    if ((bh = get_hash_table(dev, block)))
        return bh;
    tmp = free_list;
    do {
        if (tmp->b_count)
            continue;
        if (!bh || BADNESS(tmp) < BADNESS(bh)) {
            bh = tmp;
            if (!BADNESS(tmp))
                break;
        }
    /* and repeat untill we find something good */
    } while ((tmp = tmp->b_next_free) != free_list);
    if (!bh) {
        sleep_on(&buffer_wait);
        goto repeat;
    }
    wait_on_buffer(bh);
    if (bh->b_count)
        goto repeat;
    while (bh->b_dirt) {
        sync_dev(bh->b_dev);
        wait_on_buffer(bh);
        if (bh->b_count)
            goto repeat;
    }
    /* NOTE! While we slept waiting for this block, somebody else might */
    /* already have added "this" block to the cache. check it */
    if (find_buffer(dev, block))
        goto repeat;
    /* OK, FINALLY we know that this buffer is the only one of it's kind */
    /* and that it's unused (b_count = 0), unlocked (b_lock=0), and clean */
    bh->b_count = 1;
    bh->b_dirt  = 0;
    bh->b_uptodate = 0;
    remove_from_queues(bh);
    bh->b_dev = dev;
    bh->b_blocknr = block;
    insert_into_queues(bh);
    return bh;
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
struct buffer_head *breada(int dev, int first, ...)
{
    va_list args;
    struct buffer_head *bh, *tmp;

    va_start(args, first);
    if (!(bh = getblk(dev, first)))
        panic("bread: getblk returned NULL\n");
    if (!bh->b_uptodate)
        ll_rw_block(READ, bh);
    while ((first = va_arg(args, int)) >= 0) {
        tmp = getblk(dev, first);
        if (tmp) {
            if (!tmp->b_uptodate)
                ll_rw_block(READA, bh);
            tmp->b_count--;
        }
    }
    va_end(args);
    wait_on_buffer(bh);
    if (bh->b_uptodate)
        return bh;
    brelse(bh);
    return NULL;
}

static void inline invalidate_buffers(int dev)
{
    int i;
    struct buffer_head *bh;

    bh = start_buffer;
    for (i = 0; i < NR_BUFFERS; i++, bh++) {
        if (bh->b_dev != dev)
            continue;
        wait_on_buffer(bh);
        if (bh->b_dev == dev)
            bh->b_uptodate = bh->b_dirt = 0;
    }
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that foppies/whatever are
 * special.
 */
void check_disk_change(int dev)
{
    int i;

    if (MAJOR(dev) != 2)
        return;
    if (!floppy_change(dev & 0x03))
        return;
    for (i = 0; i < NR_SUPER; i++)
        if (super_block[i].s_dev == dev)
            put_super(super_block[i].s_dev);
    invalidate_inodes(dev);
    invalidate_buffers(dev);
}

int sys_sync(void)
{
    int i;
    struct buffer_head * bh;

    sync_inodes();          /* write out inodes into buffers */
    bh = start_buffer;
    for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
        wait_on_buffer(bh);
        if (bh->b_dirt)
            ll_rw_block(WRITE,bh);
    }
    return 0;
}

/*
 * bread_page reads four buffers into memory at the descired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for me one to be read, and then another
 * etc.
 */
void bread_page(unsigned long address, int dev, int b[4])
{
    struct buffer_head *bh[4];
    int i;

    for (i = 0; i < 4; i++)
        if (b[i]) {
            if ((bh[i] = getblk(dev, b[i])))
                if (!bh[i]->b_uptodate)
                    ll_rw_block(READ, bh[i]);
        } else
            bh[i] = NULL;
    for (i = 0; i < 4; i++, address += BLOCK_SIZE)
    if (bh[i]) {
        wait_on_buffer(bh[i]);
        if (bh[i]->b_uptodate)
            COPYBLK((unsigned long)bh[i]->b_data, address);
        brelse(bh[i]);
    }
}
