// user/getpid.c — Lab pgtbl 加速版 getpid
// 直接读 USYSCALL 页中的 pid，无需进入内核
//
// 这个文件有两种用途：
//  1. 作为库函数 getpid()，供其他用户程序链接使用（提供 int getpid(void)）
//  2. 编译为独立测试程序 _getpid（提供 main()），用 -DGETPID_MAIN 启用
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"

struct usyscall;  // 来自 kernel/proc.h

// 返回当前进程 PID（极快，不走 syscall）
int
getpid(void)
{
  struct usyscall *u = (struct usyscall *)USYSCALL;
  return u->pid;
}

#ifdef GETPID_MAIN
int
main(int argc, char *argv[])
{
  int pid = getpid();
  printf("getpid (fast): %d\n", pid);
  exit(0);
}
#endif
