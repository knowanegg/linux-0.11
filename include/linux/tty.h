#ifndef _TTY_H_
#define _TTY_H_

#include <termios.h>

#define TTY_BUF_SIZE 1024

struct tty_queue {
	unsigned long data;
	unsigned long head;
	unsigned long tail;
	struct task_struct *proc_list;
	char buf[TTY_BUF_SIZE];
};

struct tty_struct {
	struct termios termios;
	int pgrp;
	int stopped;
	void (*write) (struct tty_struct * tty);
	struct tty_queue read_q;
	struct tty_queue write_q;
	struct tty_queue secondary;
};

/*  intr=^C     quit=^|     erase=del   kill=^U
    eof=^D      vtime=\0    vmin=\1     sxtc=\0
    start=^Q    stop=^S     susp=^Z     eol=\0
    reprint=^R  discard=^U  werase=^W   lnext=^V
    eol2=\0
*/
#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

#define INC(a)   ((a) = ((a)+1) & (TTY_BUF_SIZE-1))
#define DEC(a)   ((a) = ((a)-1) & (TTY_BUF_SIZE-1))
#define EMPTY(a) ((a).head == (a).tail)
#define LEFT(a)  (((a).tail - (a).head - 1) & (TTY_BUF_SIZE - 1))
#define LAST(a)  ((a).buf[(TTY_BUF_SIZE - 1) & ((a).head - 1)])
#define FULL(a)  (!LEFT(a))
#define CHARS(a) (((a).head - (a).tail) & (TTY_BUF_SIZE - 1)) // 计算有多少char
#define GETCH(queue, c) \
	(void)({c=(queue).buf[(queue).tail];INC((queue).tail);})
#define PUTCH(c, queue) \
	(void)({(queue).buf[(queue).head]=(c);INC((queue).head);})

#define INTR_CHAR(tty)    ((tty)->termios.c_cc[VINTR])
#define QUIT_CHAR(tty)    ((tty)->termios.c_cc[VQUIT])
#define ERASE_CHAR(tty)   ((tty)->termios.c_cc[VERASE])
#define KILL_CHAR(tty)    ((tty)->termios.c_cc[VKILL])
#define EOF_CHAR(tty)     ((tty)->termios.c_cc[VEOF])
#define START_CHAR(tty)   ((tty)->termios.c_cc[VSTART])
#define STOP_CHAR(tty)    ((tty)->termios.c_cc[VSTOP])
#define SUSPEND_CHAR(tty) ((tty)->termios.c_cc[VSUSP])

void con_write(struct tty_struct *);
void rs_write(struct tty_struct *);
void con_init(void);
void rs_init(void);
void tty_init(void);

void copy_to_cooked(struct tty_struct *);

extern struct tty_struct tty_table[];
#endif
