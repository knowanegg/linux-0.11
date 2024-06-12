#ifndef _BLK_H_
#define _BLK_H_

/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may ues only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-machanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too may (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
#define NR_REQUEST  32


#define NR_BLK_DEV   7
/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
// 请求结构,既然放在这个blk.h下面了，就理解为块设备的请求吧
// request代表一次读盘请求
struct request {
    int dev;    // dev表示设备号, -1就代表空闲
    int cmd;    // 表示READ还是WRITE
    int errors; // 表示操作时产生的错误次数
    unsigned long sector;   // 操作的起始扇区
    unsigned long nr_sectors;   // 操作的扇区数
    char *buffer;   // 数据缓冲区，也就是读盘之后数据存在哪里
    struct task_struct *waiting;    // 是一个task_struct结构，可以表示一个进程，表示是哪个进程发起了这个请求
    struct buffer_head *bh;     // 缓冲区头指针
    struct request *next;       // 下一个请求
};
// 块设备结构体
struct blk_dev_struct {
    // 你看这个成员，request_fn没有在任何地方声明，奇怪不
    // 其实这不是 void struct a 这种，而是一个函数
    // void request_fn(void){}，这样就理解了吧
    // 这里第一个成员是一个函数指针，叫做request_fn
    void (*request_fn) (void);
    struct request *current_request;
};

static inline int IN_ORDER(struct request *s1, struct request *s2)
{
	if (s1->cmd < s2->cmd || (s1->cmd == s2->cmd && s1->dev < s2->dev)
		|| (s1->cmd == s2->cmd && s1->dev == s2->dev
			&& s1->sector < s2->sector))
		return 1;

	return 0;
}

extern struct blk_dev_struct blk_dev[];
extern struct task_struct *wait_for_request;

#ifdef CONFIG_HARDDISK
extern void (*do_hd)(void);
#endif

#endif
