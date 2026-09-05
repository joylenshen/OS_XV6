// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.

// ===== Lab cow: 引用计数 =====
static int refcount[(PHYSTOP - KERNBASE) / PGSIZE];
static struct spinlock refcount_lock;

static int
refcount_index(uint64 pa)
{
  return (int)((pa - KERNBASE) / PGSIZE);
}

// 增加引用计数（被调用时 caller 必须不持有任何 kmem 锁）
void
increment_refcount(uint64 pa)
{
  acquire(&refcount_lock);
  refcount[refcount_index(pa)]++;
  release(&refcount_lock);
}

// 减少引用计数，返回新值（被调用时 caller 必须不持有任何 kmem 锁）
int
decrement_refcount(uint64 pa)
{
  int ret;
  acquire(&refcount_lock);
  ret = --refcount[refcount_index(pa)];
  release(&refcount_lock);
  return ret;
}

// 获取引用计数
int
get_refcount(uint64 pa)
{
  int ret;
  acquire(&refcount_lock);
  ret = refcount[refcount_index(pa)];
  release(&refcount_lock);
  return ret;
}

// ===== Lab lock: per-CPU 空闲链表 =====
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit(void)
{
  initlock(&refcount_lock, "refcount");
  for (int i = 0; i < NCPU; i++) {
    // 每个 CPU 一个独立的分配锁，避免单锁成为瓶颈
    // 名称只用静态字符串，避免使用 snprintf
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa.
void
kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Lab cow: 引用计数降到 0 才真正释放页面（COW共享页不能直接memset）
  if (decrement_refcount((uint64)pa) > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  // Lab lock: 释放到当前 CPU 的空闲链表
  int cpu = cpuid();
  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
void *
kalloc(void)
{
  struct run *r;

  // Lab lock: 先从当前 CPU 的空闲链表分配
  int cpu = cpuid();
  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if (r)
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);

  if (r) {
    memset((char *)r, 5, PGSIZE);
    refcount[refcount_index((uint64)r)] = 1;  // 初始化为 1（无锁：仅 kalloc 调用者持有 kmem[cpu] 锁）
    return (void *)r;
  }

  // Lab lock: 当前 CPU 链表为空，偷取其他 CPU 的
  for (int i = 0; i < NCPU; i++) {
    if (i == cpu) continue;
    acquire(&kmem[i].lock);
    r = kmem[i].freelist;
    if (r)
      kmem[i].freelist = r->next;
    release(&kmem[i].lock);
    if (r) {
      memset((char *)r, 5, PGSIZE);
      refcount[refcount_index((uint64)r)] = 1;
      return (void *)r;
    }
  }

  return 0;  // 内存耗尽
}

// Lab syscall: 统计所有 CPU 的空闲页总字节数
uint64
kcollect_freemem(void)
{
  struct run *r;
  uint64 nfree = 0;
  for (int i = 0; i < NCPU; i++) {
    acquire(&kmem[i].lock);
    for (r = kmem[i].freelist; r; r = r->next)
      nfree++;
    release(&kmem[i].lock);
  }
  return nfree * PGSIZE;
}
