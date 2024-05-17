/*
 * linux/kernel/signal.c
 *
 *  (C) 1991 Linux Torvalds
 */
#include <signal.h>

#include <asm/segment.h>
#include <linux/sched.h>

extern void do_exit(int error_code);

// 这里的signr是前面计算出的刨除block位的signal里面最低一位为1但是置为0的位置
void do_signal(long signr, long eax, long ebx, long ecx, long edx,
	       long fs, long es, long ds,
	       long eip, long cs, long eflags, unsigned long *esp, long ss)
{
    unsigned long sa_handler;  // sa : signal action
    long old_eip = eip;        // 保留旧的eip，这是要干什么？
    struct sigaction * sa = current->sigaction + signr - 1; // 找当前进程处理这个信号的action
    int longs;
    unsigned long *tmp_esp;   // 临时esp寄存器保存

    sa_handler = (unsigned long)sa->sa_handler;
    if (sa_handler == 1)  // 如果信号句柄为 SIG_IGN（1，默认忽略句柄）则不对信号进行处理而直接返回。
        return;           // 这个SIG_IGN没搜到在哪，也没见赋值过1，可能是0.11没完善，不追究了
    if (!sa_handler) {    // 如果句柄为 SIG_DFL（0，默认处理），则根据具体的信号进行分别处理。
        if (signr == SIGCHLD)   // 如果是SIGCHLD也忽略，SIGCHLD 信号通常用于通知父进程子进程状态变化，默认处理是忽略。
            return;
        else              // 其他信号报错退出
            do_exit(1 << (signr - 1));
    }
    if (sa->sa_flags & SA_ONESHOT)  // ONESHOT表示就执行一次，执行完置空
        sa->sa_handler = NULL;      // 这里置空不会找不到handler，因为已经赋值给了局部变量sa_handler保存
    *(&eip) = sa_handler;           // eip设置成sa_handler保存的地址，跳到handler执行。
    longs = (sa->sa_flags & SA_NOMASK) ? 7 : 8;  // 表示在信号处理函数运行时，不自动阻塞正在处理的信号，这决定了需要保存的寄存器数量。
    *(&esp) -= longs;               // 上面的longs决定了esp，也就是决定了进栈参数是7个还是8个，差的是不自动阻塞的信号
    verify_area(esp, longs * 4);  // 这里的esp是addr，longs*4是size
    tmp_esp = esp;
    put_fs_long((long)sa->sa_restorer, tmp_esp++);
    put_fs_long(signr, tmp_esp++);
    if (!(sa->sa_flags & SA_NOMASK))
        put_fs_long(current->blocked, tmp_esp++);
    put_fs_long(eax, tmp_esp++);
    put_fs_long(ecx, tmp_esp++);
    put_fs_long(edx, tmp_esp++);
    put_fs_long(eflags, tmp_esp++);
    put_fs_long(old_eip, tmp_esp++);
    current->blocked |= sa->sa_mask;
}

int sys_signal(int signum, long handler, long restorer)
{
    struct sigaction tmp;

    if (signum < 1 || signum > 32 || signum == SIGKILL)
        return -1;
    tmp.sa_handler = (void (*)(int))handler;
    tmp.sa_mask = 0;
    tmp.sa_flags = SA_ONESHOT | SA_NOMASK;
    tmp.sa_restorer = (void (*)(void)) restorer;
    handler = (long)current->sigaction[signum - 1].sa_handler;
    current->sigaction[signum - 1] = tmp;
    return handler;
}

static inline void get_new(char *from, char *to)
{
    int i;

    for (i = 0; i < sizeof(struct sigaction); i++)
        *(to++) = get_fs_byte(from++);
}

static inline void save_old(char *from, char *to)
{
    int i;

    verify_area(to, sizeof(struct sigaction));
    for (i = 0; i < sizeof(struct sigaction); i++) {
        put_fs_byte(*from, to);
        from++;
        to++;
    }
}

int sys_sigaction(int signum, const struct sigaction *action,
                  struct sigaction *oldaction)
{
    struct sigaction tmp;

    if (signum < 1 || signum > 32 || signum == SIGKILL)
        return -1;
    tmp = current->sigaction[signum - 1];
    get_new((char *)action,
            (char *)(signum - 1 + current->sigaction));
    if (oldaction)
        save_old((char *)&tmp, (char *)oldaction);
    if (current->sigaction[signum - 1].sa_flags & SA_NOMASK)
        current->sigaction[signum - 1].sa_mask = 0;
    else
        current->sigaction[signum - 1].sa_mask |= (1 << (signum - 1));
    return 0;
}

int sys_sgetmask(void)
{
    return current->blocked;
}

int sys_ssetmask(int newmask)
{
    int old = current->blocked;

    current->blocked = newmask & ~(1 << (SIGKILL - 1));
    return old;
}
