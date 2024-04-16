#ifndef _SIGNAL_H_
#define _SIGNAL_H_

typedef int sig_atomic_t;
typedef unsigned int sigset_t;	/* 32 bits */

#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIOT      6
#define SIGUNUSED   7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1     10
#define SIGSEGV     11
#define SIGUSR2     12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTKFLT   16
#define SIGCHLD     17
#define SIGCONT     18
#define SIGSTOP     19
#define SIGTSTP     20
#define SIGTTIN     21
#define SIGTTOU     22

// 是一个系统调用，用于检查或修改与指定信号相关联的处理动作。
// 相比于旧的 signal 函数，sigaction 提供了更多的控制能力，允许程序以更细致的方式管理信号和信号处理。
struct sigaction {
	void (*sa_handler) (int); // handler处理程序
	sigset_t sa_mask;  // 32位掩码
	int sa_flags;  // 标志位
	void (*sa_restorer) (void); // restorer方法
};

#define SA_NOCLDSTOP 1
#define SA_NOMASK    0x40000000
#define SA_ONESHOT   0x80000000

#endif
