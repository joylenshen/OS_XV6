// Lab pgtbl: 测试 pgaccess 系统调用
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"

int main(void)
{
  uint64 base = (uint64)sbrk(4 * PGSIZE);
  if (base == (uint64)(char *)SBRK_ERROR) {
    printf("sbrk failed\n");
    exit(1);
  }

  // 访问第 0 页和第 2 页
  volatile char *p0 = (char *)base;
  volatile char *p2 = (char *)(base + 2 * PGSIZE);
  (void)p0[0];
  (void)p2[PGSIZE / 2];

  int *maskbuf = (int *)(base + 3 * PGSIZE);

  int ret = pgaccess((void *)base, 3, maskbuf);
  if (ret < 0) {
    printf("pgaccess failed\n");
    exit(1);
  }

  printf("mask = %d (bits 0 and 2 set = 5)\n", *maskbuf);
  if (*maskbuf == 5) {
    printf("pgaccess OK\n");
    exit(0);
  } else {
    printf("pgaccess FAILED: expected 5, got %d\n", *maskbuf);
    exit(1);
  }
}
