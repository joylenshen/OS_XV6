#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from, and returns to, trampoline.S
// return value is user satp for trampoline.S to switch to.
//
uint64
usertrap(void)
{
  int which_dev = 0;

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec); //DOC: kernelvec

  struct proc *p = myproc();

  // save user program counter.
  p->trapframe->epc = r_sepc();

  if (r_scause() == 8) {
    // system call

    if (killed(p))
      kexit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if ((which_dev = devintr()) != 0) {
    // ok
  } else if (r_scause() == 15) {
    // Lab cow: Store page fault (scause=15) - 可能是COW页面
    uint64 va = r_stval();
    pte_t *pte = walk(p->pagetable, va, 0);

    if (pte && (*pte & PTE_V) && (*pte & PTE_U) && ((*pte & PTE_W) == 0)) {
      // 这是一个COW页面 - 需要复制
      uint64 pa = PTE2PA(*pte);
      uint64 new_pa = (uint64)kalloc();
      if (new_pa == 0) {
        // 内存不足，杀掉进程
        setkilled(p);
      } else {
        // 复制内容到新页
        memmove((void *)new_pa, (void *)pa, PGSIZE);
        // 递减原页引用计数
        decrement_refcount(pa);
        // 设置新页为可写
        *pte = PA2PTE(new_pa) | PTE_FLAGS(*pte) | PTE_W;
        sfence_vma();
      }
    } else if (vmfault(p->pagetable, p->sz, va, 0) == 0) {
      // page fault on lazily-allocated page - 如果也处理不了
      printk("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
      printk("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
      setkilled(p);
    }
  } else if (r_scause() == 13) {
    // Lab cow/mmap: Load page fault (scause=13) - 懒分配 / mmap
    uint64 va = r_stval();
    // vmfault 返回 0 = 懒分配失败；返回非0 = 已成功分配，跳过 mmap 处理
    if (vmfault(p->pagetable, p->sz, va, 1) == 0) {
      // Lab mmap: 检查是否是 mmap 区域的 page fault
      int is_mmap_fault = 0;
      for (int i = 0; i < NVMA; i++) {
        if (p->vmas[i].valid && va >= p->vmas[i].addr &&
            va < p->vmas[i].addr + p->vmas[i].length) {
          // mmap page fault: 分配物理页，从文件读入
          char *mem = kalloc();
          if (mem == 0) {
            setkilled(p);
            break;
          }
          memset(mem, 0, PGSIZE);
          uint off = p->vmas[i].offset + (va - p->vmas[i].addr);
          ilock(p->vmas[i].file->ip);
          readi(p->vmas[i].file->ip, 0, (uint64)mem, off, PGSIZE);
          iunlock(p->vmas[i].file->ip);
          int perm = PTE_U | PTE_R;
          if (p->vmas[i].prot & PROT_WRITE)
            perm |= PTE_W;
          if (mappages(p->pagetable, va, PGSIZE, (uint64)mem, perm) != 0) {
            kfree(mem);
            setkilled(p);
            break;
          }
          is_mmap_fault = 1;
          break;
        }
      }
      if (!is_mmap_fault) {
        printk("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
        printk("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
        setkilled(p);
      }
    }
  } else {
    printk("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printk("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if (killed(p))
    kexit(-1);

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2) {
    // Lab traps: 处理 sigalarm
    if (p->alarm_interval > 0 && !p->alarm_goingoff) {
      p->alarm_ticks--;
      if (p->alarm_ticks <= 0) {
        p->alarm_goingoff = 1;
        // 保存当前 trapframe
        *p->alarm_trapframe = *p->trapframe;
        // 设置 PC 为 handler
        p->trapframe->epc = (uint64)p->alarm_handler;
        // 重置 ticks
        p->alarm_ticks = p->alarm_interval;
      }
    }
    yield();
  }

  prepare_return();

  // the user page table to switch to, for trampoline.S
  uint64 satp = MAKE_SATP(p->pagetable);

  // return to trampoline.S; satp value in a0.
  return satp;
}

//
// set up trapframe and control registers for a return to user space
//
void
prepare_return(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(). because a trap from kernel
  // code to usertrap would be a disaster, turn off interrupts.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if ((which_dev = devintr()) == 0) {
    // interrupt or trap from an unknown source
    printk("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(),
           r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if (cpuid() == 0) {
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if (scause == 0x8000000000000009L) {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ) {
      uartintr();
    } else if (irq == VIRTIO0_IRQ) {
      virtio_disk_intr();
    } else if (irq) {
      printk("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  } else if (scause == 0x8000000000000005L) {
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}
