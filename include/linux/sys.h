extern int sys_setup(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_read(void);
extern int sys_write(void);
extern int sys_open(void);
extern int sys_close(void);
extern int sys_waitpid(void);
extern int sys_creat(void);
extern int sys_link(void);
extern int sys_unlink(void);
extern int sys_execve(void);
extern int sys_chdir(void);
extern int sys_time(void);
extern int sys_mknod(void);
extern int sys_chmod(void);
extern int sys_chown(void);
extern int sys_break(void);
extern int sys_stat(void);
extern int sys_lseek(void);
extern int sys_getpid(void);
extern int sys_mount(void);
extern int sys_umount(void);
extern int sys_setuid(void);
extern int sys_getuid(void);
extern int sys_stime(void);
extern int sys_ptrace(void);
extern int sys_alarm();
extern int sys_fstat(void);
extern int sys_pause(void);
extern int sys_utime(void);
extern int sys_stty(void);
extern int sys_gtty(void);
extern int sys_access(void);
extern int sys_nice();
extern int sys_ftime(void);
extern int sys_sync(void);
extern int sys_kill(void);
extern int sys_rename(void);
extern int sys_mkdir(void);
extern int sys_rmdir(void);
extern int sys_dup(void);
extern int sys_pipe(void);
extern int sys_times(void);
extern int sys_prof(void);
extern int sys_brk(void);
extern int sys_setgid(void);
extern int sys_getgid(void);
extern int sys_signal(void);
extern int sys_geteuid(void);
extern int sys_getegid(void);
extern int sys_acct(void);
extern int sys_phys(void);
extern int sys_lock(void);
extern int sys_ioctl(void);
extern int sys_fcntl(void);
extern int sys_mpx(void);
extern int sys_setpgid(void);
extern int sys_ulimit(void);
extern int sys_uname(void);
extern int sys_umask(void);
extern int sys_chroot(void);
extern int sys_ustat(void);
extern int sys_dup2(void);
extern int sys_getppid(void);
extern int sys_getpgrp(void);
extern int sys_setsid(void);
extern int sys_sigaction(void);
extern int sys_sgetmask(void);
extern int sys_ssetmask(void);
extern int sys_setreuid(void);
extern int sys_setregid(void);
#ifdef CONFIG_DEBUG_SYSCALL_OPEN0
extern int sys_d_open(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CLOSE0
extern int sys_d_close(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_READ0
extern int sys_d_read(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_WRITE0
extern int sys_d_write(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_DUP0
extern int sys_d_dup(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FORK0
extern int sys_d_fork(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_EXECVE0
extern int sys_d_execve(void); 
#endif
#ifdef CONFIG_DEBUG_BINARY_AOUT
extern int sys_d_execve(void); 
#endif
#ifdef CONFIG_DEBUG_BINARY_ELF
extern int sys_d_parse_elf(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STACK
extern int sys_d_stack(void); 
#endif
#ifdef CONFIG_DEBUG_SEGMENT_FS
extern int sys_d_fs(void); 
#endif
#ifdef CONFIG_DEBUG_MMU
extern int sys_d_mmu(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CREAT
extern int sys_d_creat(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_LINK
extern int sys_d_link(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UNLINK
extern int sys_d_unlink(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MKDIR
extern int sys_d_mkdir(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_RMDIR
extern int sys_d_rmdir(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MKNOD
extern int sys_d_mknod(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ACCESS
extern int sys_d_access(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ACCT
extern int sys_d_acct(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ALARM
extern int sys_d_alarm(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHDIR
extern int sys_d_chdir(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHMOD
extern int sys_d_chmod(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHOWN
extern int sys_d_chown(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UTIME
extern int sys_d_utime(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHROOT
extern int sys_d_chroot(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_USTAT
extern int sys_d_ustat(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FSTAT
extern int sys_d_fstat(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STAT
extern int sys_d_stat(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETPID
extern int sys_d_getpid(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETUID
extern int sys_d_getuid(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PAUSE
extern int sys_d_pause(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_NICE
extern int sys_d_nice(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETGID
extern int sys_d_getgid(void); 
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETEUID
extern int sys_d_geteuid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETPPID
extern int sys_d_getppid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_TIME
extern int sys_d_time(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STIME
extern int sys_d_stime(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_TIMES
extern int sys_d_times(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FTIME
extern int sys_d_ftime(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ULIMIT
extern int sys_d_ulimit(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MPX
extern int sys_d_mpx(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_LOCK
extern int sys_d_lock(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PHYS
extern int sys_d_phys(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PROF
extern int sys_d_prof(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GTTY
extern int sys_d_gtty(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STTY
extern int sys_d_stty(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PTRACE
extern int sys_d_ptrace(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETPGRP
extern int sys_d_getpgrp(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETSID
extern int sys_d_setsid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UMASK
extern int sys_d_umask(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UNAME
extern int sys_d_uname(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETPGID
extern int sys_d_setpgid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETGID
extern int sys_d_setgid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETREGID
extern int sys_d_setregid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_BRK
extern int sys_d_brk(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETUID
extern int sys_d_setuid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETREUID
extern int sys_d_setreuid(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_BREAK
extern int sys_d_break(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_RENAME
extern int sys_d_rename(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FCNTL
extern int sys_d_fcntl(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_DUP2
extern int sys_d_dup2(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_LSEEK
extern int sys_d_lseek(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MOUNT
extern int sys_d_mount(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UMOUNT
extern int sys_d_umount(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETUP
extern int sys_d_setup(void);
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SIGNAL
extern int sys_d_signal(void);
#endif

fn_ptr sys_call_table[] = {
sys_setup, /* system setup */
sys_exit,  /* terminate the current process */
sys_fork,  /* create a child process */
sys_read,  /* read from a file descriptor */
sys_write, /* write to a file descriptor */
sys_open, /* open and possibly create a file */
sys_close, /* closes a file on the host system */
sys_waitpid, /* wait for process termination */
sys_creat, /* create a file or device */
sys_link, /* make a new name for a file */
sys_unlink, /* delete a name and possibly the file it refers to */
sys_execve, /* execute program */
sys_chdir, /* change working directory */
sys_time, /* get time in seconds */
sys_mknod, /* create a directory or special or ordinary file */
sys_chmod, /* change permissions of a file */
sys_chown, /* change ownership of a file */
sys_break, /* call exists only for compatibility */
sys_stat, /* --- */
sys_lseek, /* reposition read/write file offset */
sys_getpid, /* get process identification */
sys_mount, /* mount filesystems */
sys_umount, /* unmount filesystem */
sys_setuid, /* set user identity */
sys_getuid, /* get user identity */
sys_stime, /* set time */
sys_ptrace, /* process trace */
sys_alarm, /* set an alarm clock for delivery of a signal */
sys_fstat, /* */
sys_pause, /* wait for signal */
sys_utime, /* change access and/or modification times of an inode */
sys_stty, /* set mode of typewriter */
sys_gtty, /* get typewriter status */
sys_access, /* check user's permissions for a file */
sys_nice, /* change process priority */
sys_ftime, /* --- */
sys_sync, /* write buffer into disk */
sys_kill, /* send a signal to a process  */
sys_rename, /* Renames a specified file */
sys_mkdir, /* Create a directory */
sys_rmdir, /* Remove a directory */
sys_dup, /* duplicate an open file descriptor */
sys_pipe, /* create an interprocess channel */
sys_times, /* file access and modification times structure */
sys_prof, /* profiling library */
sys_brk, /* allocates memory right behind application image in memory */
sys_setgid, /* Set the numerical group id */
sys_getgid, /* Get the numerical group id */
sys_signal, /* signal handling */
sys_geteuid, /* get current task euid */
sys_getegid, /* get current task egid */
sys_acct, /* enable/disable process accounting */
sys_phys, /* --- */
sys_lock, /* --- */
sys_ioctl, /* control device */
sys_fcntl, /* manipulate file descriptor */
sys_mpx, /* -- */
sys_setpgid, /* simplify pid/ns interaction */
sys_ulimit, /* -- */
sys_uname, /* get name and information about current kernel */
sys_umask, /* get current task umask */
sys_chroot, /* change root directory */
sys_ustat, /* -- */
sys_dup2, /* duplicate a file descriptor */
sys_getppid, /* get current father pid */
sys_getpgrp, /* get pgrp of current task */
sys_setsid, /* creates a session and sets the process group ID */
sys_sigaction, /* examine and change a signal action */
sys_sgetmask, /* manipulation of signal mask */
sys_ssetmask, /* -- */
sys_setreuid, /* set real and/or effective user or group ID */
sys_setregid, /* Set the numerical group id */
#ifdef CONFIG_DEBUG_SYSCALL_OPEN0
sys_d_open,  /* Open file/directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CLOSE0
sys_d_close,  /* Close file/directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_READ0
sys_d_read,  /* Read data from file/directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_WRITE0
sys_d_write,  /* Write data to file/directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_DUP0
sys_d_dup,  /* duplicate an open file descriptor */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FORK0
sys_d_fork,  /* create a new process */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_EXECVE0
sys_d_execve,  /* execute program */
#endif
#ifdef CONFIG_DEBUG_BINARY_AOUT
sys_d_execve,  /* execute program */
#endif
#ifdef CONFIG_DEBUG_BINARY_ELF
sys_d_parse_elf,  /* parse elf format */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STACK
sys_d_stack,  /* dump stack map on trigger system call */
#endif
#ifdef CONFIG_DEBUG_SEGMENT_FS
sys_d_fs,  /* Exchange data between kernel and userland */
#endif
#ifdef CONFIG_DEBUG_MMU
sys_d_mmu,  /* physical, linaer, logica and virtual address */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CREAT
sys_d_creat,  /* create a new file or rewrite an existing one */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_LINK
sys_d_link,  /* link a file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UNLINK
sys_d_unlink,  /* remove a link file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MKDIR
sys_d_mkdir,  /* create a new directory and specify access mode */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_RMDIR
sys_d_rmdir,  /* remove a existed directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MKNOD
sys_d_mknod,  /* create a regular file or director or special file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ACCESS
sys_d_access,  /* check user's permissions for a file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ACCT
sys_d_acct,  /* enable/disable process accounting */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ALARM
sys_d_alarm,  /* set an alarm clock for delivery of a signal */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHDIR
sys_d_chdir,  /* change working directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHMOD
sys_d_chmod,  /* change permissions of a file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHOWN
sys_d_chown,  /* change ownership of a file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UTIME
sys_d_utime,  /* change access and/or modification times of an inode */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_CHROOT
sys_d_chroot,  /* change root directory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_USTAT
sys_d_ustat,   /* -- unknown -- */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FSTAT
sys_d_fstat, /* -- unknown -- */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STAT
sys_d_stat, /* -- unknown -- */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETPID
sys_d_getpid, /* get process identification */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETUID
sys_d_getuid, /* get current task euid */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PAUSE
sys_d_pause, /* wait for signal */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_NICE
sys_d_nice, /* change process priority */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETGID
sys_d_getgid, /* Get the numerical group id */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETEUID
sys_d_geteuid, /* get current task euid */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETPPID
sys_d_getppid, /* get the parent process ID */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_TIME
sys_d_time, /* get time in seconds */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STIME
sys_d_stime, /* set time in seconds */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_TIMES
sys_d_times, /* file access and modification times structure */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FTIME
sys_d_ftime, /* get date and time (LEGACY) */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_ULIMIT
sys_d_ulimit, /* get and set process limits */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MPX
sys_d_mpx, /* unknow system call */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_LOCK
sys_d_lock, /* unknow system call */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PHYS
sys_d_phys, /* unknow system call */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PROF
sys_d_prof, /* unknow system call */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GTTY
sys_d_gtty, /* get typewriter status */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_STTY
sys_d_stty, /* set typewriter status */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_PTRACE
sys_d_ptrace, /* process trace */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_GETPGRP
sys_d_getpgrp, /* get pgrp of current task */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETSID
sys_d_setsid, /* creates a session and sets the process group ID */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UMASK
sys_d_umask, /* get current task umask */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UNAME
sys_d_uname, /* get name and information about current kernel */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETPGID
sys_d_setpgid, /* simplify pid/ns interaction */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETGID
sys_d_setgid, /* Set the numerical group id */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETREGID
sys_d_setregid, /* Set the numerical group id */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_BRK
sys_d_brk, /* allocates memory right behind application image in memory */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETUID
sys_d_setuid, /* set user identity */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETREUID
sys_d_setreuid, /* set real and/or effective user or group ID */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_BREAK
sys_d_break, /* call exists only for compatibility */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_RENAME
sys_d_rename, /* Renames a specified file */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_FCNTL
sys_d_fcntl, /* manipulate file descriptor */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_DUP2
sys_d_dup2, /* duplicate a file descriptor */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_LSEEK
sys_d_lseek, /* reposition read/write file offset */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_MOUNT
sys_d_mount, /* mount filesystems */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_UMOUNT
sys_d_umount, /* unmount filesystems */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SETUP
sys_d_setup, /* system setup */
#endif
#ifdef CONFIG_DEBUG_SYSCALL_SIGNAL
sys_d_signal, /* signal handling */
#endif
};
