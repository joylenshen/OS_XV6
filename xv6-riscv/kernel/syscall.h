// System call numbers
#define SYS_fork   1
#define SYS_exit   2
#define SYS_wait   3
#define SYS_pipe   4
#define SYS_read   5
#define SYS_kill   6
#define SYS_exec   7
#define SYS_fstat  8
#define SYS_chdir  9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_pause  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
#define SYS_sync   22
// Lab syscall additions
#define SYS_trace    23
#define SYS_sysinfo  24
// Lab traps additions
#define SYS_sigalarm  25
#define SYS_sigreturn 26
// Lab pgtbl additions
#define SYS_pgaccess 27
// Lab fs additions
#define SYS_symlink  28
// Lab mmap additions
#define SYS_mmap     29
#define SYS_munmap   30
// Lab util additions
#define SYS_sleep    31
