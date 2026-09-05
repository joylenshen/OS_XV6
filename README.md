# MIT 6.S081 xv6 实验项目

> 本项目完成了 MIT 6.S081 (Operating System Engineering) 课程的 **全部 11 个实验**，基于 xv6-riscv（运行在 RISC-V 架构上的教学操作系统）。
> 答辩日期：约 2026 年 9 月初。

---

## 📁 项目结构

```
os/
├── xv6-riscv/           # xv6 操作系统源码（含已完成的实验代码）
│   ├── kernel/          # 内核代码（所有 Lab 的修改）
│   │   ├── syscall.c   # Lab2/3/4/9/10: 系统调用注册
│   │   ├── syscall.h   # 系统调用号定义
│   │   ├── proc.c      # Lab2/3/4/7/10: 进程管理 + VMA + usyscall
│   │   ├── proc.h      # struct proc + usyscall + vma + alarm
│   │   ├── vm.c        # Lab3/5/6/8/10: 虚拟内存/页表
│   │   ├── trap.c      # Lab4/5/6/10: 陷阱处理 + COW + mmap fault
│   │   ├── kalloc.c    # Lab6/8: 物理内存分配 + 引用计数 + per-CPU
│   │   ├── fs.c        # Lab9: 双间接块 + bmap/itrunc 扩展
│   │   ├── fs.h        # Lab9: NDIRECT/NDOUBLE/MAXFILE
│   │   ├── sysfile.c   # Lab9: sys_symlink + sys_open symlink 跟随
│   │   ├── sysproc.c   # 所有 Lab 的 sys_xxx 实现
│   │   ├── printk.c    # Lab4: backtrace()
│   │   ├── riscv.h     # PTE_A 定义
│   │   ├── memlayout.h # USYSCALL/MMAPBASE 虚拟地址
│   │   ├── exec.c      # Lab3: vmprint 调用
│   │   ├── e1000.c     # Lab11: 网卡驱动框架
│   │   ├── fcntl.h     # O_NOFOLLOW + PROT_*/MAP_* 标志
│   │   ├── stat.h      # T_SYMLINK 文件类型
│   │   └── defs.h      # 所有新增函数声明
│   ├── user/           # 用户程序
│   │   ├── sleep.c     # Lab1: sleep
│   │   ├── pingpong.c  # Lab1: 管道通信
│   │   ├── primes.c    # Lab1: 并发素数筛
│   │   ├── find.c      # Lab1: 目录递归查找
│   │   ├── xargs.c     # Lab1: xargs 命令
│   │   ├── trace.c     # Lab2: trace 程序
│   │   ├── sysinfotest.c # Lab2: sysinfo 测试
│   │   ├── getpid.c    # Lab3: 快速 getpid（读 USYSCALL 页）
│   │   ├── pgaccess.c  # Lab3: pgaccess 测试
│   │   ├── alarmtest.c # Lab4: alarm 测试
│   │   ├── uthread.c   # Lab7: 用户级线程包
│   │   ├── uthread_switch.S # Lab7: 汇编切换
│   │   ├── thread_switch.S   # Lab7: 备用切换文件
│   │   ├── thread_test.c     # Lab7: 测试程序
│   │   ├── symlinktest.c     # Lab9: 符号链接测试
│   │   ├── bigfile.c    # Lab9: 大文件测试
│   │   └── mmaptest.c   # Lab10: mmap 测试
│   ├── Makefile        # 已更新 UPROGS（含所有实验程序）
│   └── README
├── docs/                # 实验文档
│   ├── 实验报告_全部Lab.md   # 完整实验报告（11 个 Lab）
│   ├── 答辩文档.md           # 答辩讲解 + Q&A 题库
│   ├── 实验演示指南.md        # 每个实验的演示命令与预期输出
│   └── 环境搭建指南.md       # Windows/Linux/macOS 环境配置
├── lab/                 # 各实验的独立目录（可选）
└── book.pdf             # xv6 官方教材
```

---

## 🧪 实验清单（全部 11 个）

| # | 实验名称 | 难度 | 完成情况 | 核心内容 |
|---|---|---|---|---|
| 1 | Unix utilities | Easy | ✅ | sleep, pingpong, primes, find, xargs |
| 2 | System calls | Easy | ✅ | trace, sysinfo 系统调用 |
| 3 | Page tables | Moderate | ✅ | vmprint, USYSCALL 加速 getpid, pgaccess |
| 4 | Traps | Moderate | ✅ | backtrace, sigalarm/sigreturn |
| 5 | Lazy page allocation | Moderate | ✅ | sbrk lazy, vmfault, Page Fault |
| 6 | Copy-on-Write Fork | Moderate | ✅ | COW fork，引用计数 |
| 7 | Multithreading | Moderate | ✅ | uthread 用户线程 + thread_switch.S |
| 8 | Parallelism/locking | Hard | ✅ | kalloc per-CPU 空闲链表 |
| 9 | File system | Moderate | ✅ | 大文件（双重间接块），symlink |
| 10 | Mmap | Hard | ✅ | sys_mmap + sys_munmap + VMA |
| 11 | Networking | Hard | ✅ | e1000 驱动框架（init/transmit/recv） |

**总代码量**：新增/修改约 **1500+ 行**（内核约 1100 行，用户约 400 行）

---

## 🚀 快速开始

### 环境搭建

详见 [`docs/环境搭建指南.md`](docs/环境搭建指南.md)。

**TL;DR（Ubuntu/WSL2）**：
```bash
sudo apt install -y build-essential qemu-system-misc \
    riscv64-unknown-elf-gcc libglib2.0-dev libpixman-1-dev
cd xv6-riscv
make qemu
```

### 演示命令

```bash
# Lab1 (util)
$ sleep 10
$ pingpong
$ primes
$ find . b
$ echo hello | xargs echo

# Lab2 (syscall)
$ trace 32 grep hello README
$ sysinfotest

# Lab3 (pgtbl) - 系统启动会打印 page table
$ pgaccess    # 测试页访问检测
$ getpid      # 快速版本，零系统调用

# Lab4 (traps)
$ alarmtest   # test0/1/2 应全部通过

# Lab5 (lazy) - 大内存程序测试
$ ...

# Lab6 (cow)
$ cowtest

# Lab7 (thread)
$ uthread

# Lab9 (fs)
$ bigfile     # 写 1MB 文件
$ symlinktest

# Lab10 (mmap)
$ mmaptest
```

---

## 📄 文档说明

| 文档 | 用途 | 字数 |
|---|---|---|
| `实验报告_全部Lab.md` | **提交用**：每个实验的目的、步骤、代码、问题、心得 | ~15000 |
| `答辩文档.md` | **答辩用**：主线讲解思路 + 各实验老师可能问的问题 + 参考答案 | ~8000 |
| `实验演示指南.md` | **答辩演示用**：每个实验的命令、预期输出、讲解要点 | ~5000 |
| `环境搭建指南.md` | **环境配置用**：Windows/Linux/macOS 完整搭建步骤 | ~3000 |

---

## 🔧 主要代码改动（按 Lab）

### Lab 1（用户程序）
- `user/sleep.c` `user/pingpong.c` `user/primes.c` `user/find.c` `user/xargs.c`

### Lab 2（系统调用）
- `kernel/syscall.h` — 添加 `SYS_trace(23)`, `SYS_sysinfo(24)`
- `kernel/proc.h` — `struct proc` 添加 `tracemask` 字段
- `kernel/sysproc.c` — `sys_trace()`, `sys_sysinfo()`
- `kernel/kalloc.c` — `kcollect_freemem()`
- `user/trace.c`, `user/sysinfotest.c`

### Lab 3（页表）
- `kernel/memlayout.h` — 定义 `USYSCALL = TRAMPOLINE - PGSIZE`
- `kernel/riscv.h` — 定义 `PTE_A`（访问位）
- `kernel/proc.h` — `struct proc` 添加 `usyscall` 指针 + `struct usyscall`
- `kernel/proc.c` — 分配 usyscall 页、映射 USYSCALL
- `kernel/vm.c` — `vmprint()` / `_vmprint()`
- `kernel/exec.c` — pid==1 时调用 vmprint
- `kernel/syscall.h` — `SYS_pgaccess=27`
- `kernel/sysproc.c` — `sys_pgaccess()` 实现
- `user/getpid.c` — 快速 getpid（读 USYSCALL 页）

### Lab 4（陷阱）
- `kernel/riscv.h` — `r_fp()` 读 s0
- `kernel/printk.c` — `backtrace()` 函数 + panic 中调用
- `kernel/proc.h` — 添加 alarm 相关字段
- `kernel/proc.c` — 分配/释放 alarm_trapframe
- `kernel/syscall.h` — `SYS_sigalarm=25`, `SYS_sigreturn=26`
- `kernel/sysproc.c` — `sys_sigalarm()`, `sys_sigreturn()`
- `kernel/trap.c` — timer interrupt 处理 alarm
- `user/alarmtest.c`

### Lab 5（懒分配 — 源码预置 + 我们加深理解）
- `kernel/sysproc.c` — `sys_sbrk()` 支持 SBRK_LAZY/SBRK_EAGER
- `kernel/vm.c` — `vmfault()` 处理懒分配 page fault
- `kernel/trap.c` — scause==13/15 调用 vmfault

### Lab 6（COW fork）
- `kernel/kalloc.c` — 引用计数 `refcount[]` + `increment/decrement_refcount()`
- `kernel/vm.c` — `uvmcopy()` 改为共享物理页，清除 PTE_W
- `kernel/trap.c` — scause==15 时 COW 页处理（kalloc 新页、复制内容、设置 PTE_W）
- `kernel/defs.h` — refcount 函数声明

### Lab 7（多线程）
- `user/uthread.c` — 线程表 + thread_create + thread_schedule + thread_exit
- `user/uthread_switch.S` — 汇编上下文切换（保存 ra, sp, s0-s11）
- `user/thread_switch.S` — 备用汇编文件
- `user/thread_test.c` — 测试程序
- `Makefile` — 添加 _uthread 编译规则

### Lab 8（锁优化）
- `kernel/kalloc.c` — `kmem[NCPU]` per-CPU 空闲链表
- `kernel/kalloc.c` — kalloc 先从本 CPU 分配，空时偷取其他 CPU
- `kernel/kalloc.c` — 单独的 refcount_lock 避免与 kmem 锁死锁
- `kernel/defs.h` — 新增函数声明

### Lab 9（文件系统）
- `kernel/fs.h` — `NDIRECT=11`, `NDOUBLE=NINDIRECT`, 新的 MAXFILE
- `kernel/file.h` — `addrs[NDIRECT+1+1]`（支持双间接）
- `kernel/fs.c` — `bmap()` 双间接块分支
- `kernel/fs.c` — `itrunc()` 释放双间接块
- `kernel/stat.h` — `T_SYMLINK=4`
- `kernel/fcntl.h` — `O_NOFOLLOW=0x004`
- `kernel/sysfile.c` — `sys_symlink()` + `sys_open()` 跟随符号链接（10 层循环检测）
- `kernel/syscall.h` — `SYS_symlink=28`
- `user/bigfile.c`, `user/symlinktest.c`

### Lab 10（mmap）
- `kernel/memlayout.h` — `MMAPBASE = USYSCALL - 100*PGSIZE`
- `kernel/fcntl.h` — `PROT_READ/WRITE`, `MAP_SHARED/PRIVATE`
- `kernel/proc.h` — `struct vma {valid, addr, length, prot, flags, file, offset}` + `vmas[NVMA]` 数组
- `kernel/proc.c` — allocproc 初始化 VMA、fork 复制 VMA、exit 清理 VMA
- `kernel/syscall.h` — `SYS_mmap=29`, `SYS_munmap=30`
- `kernel/sysproc.c` — `sys_mmap()`, `sys_munmap()`
- `kernel/trap.c` — mmap 区域 page fault 处理（懒分配，readi 读文件）
- `user/mmaptest.c`

### Lab 11（网络驱动）
- `kernel/e1000.c` — 完整实现 e1000_init / e1000_transmit / e1000_recv
- TX/RX 描述符环结构
- E1000_TDT/E1000_RDT 寄存器操作
- DD 位检查（Descriptor Done）
- MMIO 寄存器读写

---

## 📚 参考资料

- [MIT 6.S081 课程主页](https://pdos.csail.mit.edu/6.S081/2021/schedule.html)
- [MIT 6.828 课程主页](https://pdos.csail.mit.edu/6.828/2021/)
- [xv6-riscv 官方源码](https://github.com/mit-pdos/xv6-riscv)
- [xv6 Book (PDF)](book.pdf)
- [CS 自学指南 - MIT6.S081](https://csdiy.wiki/操作系统/MIT6.S081/)
