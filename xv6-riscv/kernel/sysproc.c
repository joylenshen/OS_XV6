#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "sysinfo.h"

// Lab pgtbl: 检查用户页面访问位
uint64
sys_pgaccess(void)
{
  uint64 start;
  int len;
  uint64 mask_addr;
  struct proc *p = myproc();

  argaddr(0, &start);
  argint(1, &len);
  argaddr(2, &mask_addr);

  if (len > 64)
    return -1;

  uint64 mask = 0;
  for (int i = 0; i < len; i++) {
    uint64 va = start + i * PGSIZE;
    pte_t *pte = walk(p->pagetable, va, 0);
    if (pte == 0)
      continue;
    if ((*pte & PTE_V) == 0)
      continue;
    if (*pte & PTE_A) {
      mask |= (1 << i);
      *pte &= ~PTE_A;  // 清除访问位，以便下次检测
    }
  }

  if (copyout(p->pagetable, (uint64)p->sz, mask_addr, (char *)&mask, sizeof(mask)) < 0)
    return -1;
  return 0;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;

  // 用标准的 sleep + wakeup 实现
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Lab syscall: trace
uint64
sys_trace(void)
{
  int mask;
  argint(0, &mask);
  if (mask < 0)
    return -1;
  myproc()->tracemask = mask;
  return 0;
}

// Lab traps: sigalarm
// 设置每 n 个 tick 后调用 handler
uint64
sys_sigalarm(void)
{
  int n;
  uint64 fn;
  struct proc *p = myproc();

  argint(0, &n);
  argaddr(1, &fn);

  p->alarm_interval = n;
  p->alarm_handler = (void (*)())fn;
  p->alarm_ticks = n;
  return 0;
}

// Lab traps: sigreturn
// 从 alarm handler 返回，回复原 trapframe
uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  if (p->alarm_trapframe == 0)
    return -1;
  *p->trapframe = *p->alarm_trapframe;
  p->alarm_goingoff = 0;
  return p->trapframe->a0;  // 恢复 handler 的返回值
}

// Lab syscall: sysinfo
// 收集系统信息（空闲内存字节数 + 非 UNUSED 进程数）
uint64
sys_sysinfo(void)
{
  uint64 addr;
  struct sysinfo info;

  argaddr(0, &addr);

  info.freemem = kcollect_freemem();
  info.nproc   = kcollect_nproc();

  if (copyout(myproc()->pagetable, (uint64)myproc()->sz, addr, (char *)&info, sizeof(info)) < 0)
    return -1;
  return 0;
}

// Lab mmap: 实现 mmap 系统调用
// 用法: void *mmap(void *addr, int length, int prot, int flags, int fd, int offset);
// 简化版：只支持文件映射，懒分配物理页
uint64
sys_mmap(void)
{
  uint64 addr, length, offset;
  int prot, flags, fd;
  struct file *f;

  argaddr(0, &addr);
  argint(1, (int *)&length);
  argint(2, &prot);
  argint(3, &flags);
  argint(4, &fd);
  argaddr(5, &offset);

  // 从 fd 获取 file
  struct proc *p = myproc();
  if (fd < 0 || fd >= NOFILE || (f = p->ofile[fd]) == 0)
    return -1;

  if (length <= 0 || (prot & ~0x3) != 0 || (flags & ~0x3) != 0)
    return -1;

  // 找一个空闲的 VMA 槽
  struct vma *vma = 0;
  for (int i = 0; i < NVMA; i++) {
    if (!p->vmas[i].valid) {
      vma = &p->vmas[i];
      break;
    }
  }
  if (vma == 0) return -1;

  // 找一个未使用的虚拟地址（在 MMAPBASE 附近）
  // 简化：直接使用 MMAPBASE，每次分配递增
  static uint64 next_mmap_addr = MMAPBASE;  // 简化版：单进程
  if (next_mmap_addr + length > MMAPBASE + 100*PGSIZE)
    return -1;  // 空间不足
  addr = next_mmap_addr;
  next_mmap_addr += length;

  vma->valid = 1;
  vma->addr = addr;
  vma->length = length;
  vma->prot = prot;
  vma->flags = flags;
  vma->file = f;
  vma->offset = offset;
  filedup(f);

  return addr;
}

// Lab mmap: 实现 munmap 系统调用
uint64
sys_munmap(void)
{
  uint64 addr, length;

  argaddr(0, &addr);
  argaddr(1, &length);

  struct proc *p = myproc();
  for (int i = 0; i < NVMA; i++) {
    if (p->vmas[i].valid && p->vmas[i].addr == addr) {
      uvmunmap(p->pagetable, addr, length/PGSIZE, 1);
      // MAP_SHARED 时需要写回文件（这里简化）
      fileclose(p->vmas[i].file);
      p->vmas[i].valid = 0;
      return 0;
    }
  }
  return -1;
}

// Lab fs: 符号链接已实现于 sysfile.c
// 声明 stub 防止编译错误
extern uint64 sys_symlink(void);
