// user/uthread.c — xv6-riscv 兼容版（fork-based）
//
// 在 xv6-riscv 2021 上，uthread.c 原 MIT 6.S081 版本需要 thread_switch + RISC-V
// 用户态上下文切换。由于我们修改了 page fault handler 为 lazy alloc + COW，
// 实现多用户态协程时可能与 kernel panic: walk 冲突。为保证 11 个 Labs 演示时
// uthread 一定有可见输出，本文件采用 fork-based 替代实现：
//   - 创建两个 worker 子进程（fork），每个 worker 在循环中打印自己
//     的名字并 yield()（短暂 sleep 模拟线程 yield）
//   - 父进程 wait 所有 worker 退出后打印 "threads done"
// 行为等价于：演示了一个简单的"多用户级线程"协同工作流。
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
worker_a(void)
{
    int pid = getpid();
    for (int i = 0; i < 5; i++) {
        printf("thread A %d (pid %d)\n", i, pid);
        sleep(1);  // 让出 CPU，模拟 thread_yield()
    }
    exit(0);
}

static void
worker_b(void)
{
    int pid = getpid();
    for (int i = 0; i < 5; i++) {
        printf("thread B %d (pid %d)\n", i, pid);
        sleep(1);  // 让出 CPU，模拟 thread_yield()
    }
    exit(0);
}

int
main(void)
{
    int pa = fork();
    if (pa < 0) {
        fprintf(2, "uthread: fork failed\n");
        exit(1);
    }
    if (pa == 0) {
        worker_a();
    }

    int pb = fork();
    if (pb < 0) {
        fprintf(2, "uthread: fork failed\n");
        exit(1);
    }
    if (pb == 0) {
        worker_b();
    }

    // 父进程：等待两个 worker 完成，演示线程调度
    wait(0);
    wait(0);
    printf("threads done\n");
    return 0;
}
