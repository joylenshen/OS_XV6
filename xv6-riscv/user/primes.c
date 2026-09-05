// user/primes.c — MIT 6.S081 Lab1 util
// 并发 Sieve of Eratosthenes using pipes（参考 MIT 6.S081 官方 solution 风格）
//
// 设计概述（来自 Doug McIlroy 协同筛法）：
//   - main: 创建首个 pipe first_pipe[2]，fork。子进程作 feeder 向 first_pipe[1]
//         写 2..35 后 exit(0)；主父进程作 receiver 起点进入 sieve chain。
//   - receiver (递归调用链)：从 cur_reader 读一个 int 作为本层素数 p，
//         printf "prime %d\n"。创建 next_pipe[2]，fork 一个 worker 子进程做
//         过滤：cur_reader 中剩余的数按 n % p != 0 过滤后写到 next_pipe[1]。
//         父进程（当前 receiver）关闭 worker 写端，立即递归到下一层
//         receiver(next_pipe[0])。
//   - worker: 在循环中 read cur_reader，过滤后 write writer；read EOF 时退出。
//
// 在关键转换处使用 fprintf(2,".") 触发 uartwrite 的 sleep，从而强制调度 yield，
// 解决 xv6-riscv 单核调度下 primes 易卡死的实际问题。
#include "kernel/types.h"
#include "user/user.h"

static void
worker(int p, int reader, int writer)
{
    int n;
    while (read(reader, &n, sizeof(int)) == sizeof(int)) {
        if (n % p != 0) {
            if (write(writer, &n, sizeof(int)) != sizeof(int)) {
                exit(1);
            }
        }
    }
    close(reader);
    close(writer);
    fprintf(2, ".\n");
    exit(0);
}

static void
receiver(int reader)
{
    int p;
    if (read(reader, &p, sizeof(int)) != sizeof(int)) {
        close(reader);
        exit(0);
    }
    printf("prime %d\n", p);

    int wp[2];
    if (pipe(wp) < 0) {
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        exit(1);
    }
    if (pid == 0) {
        close(wp[0]);
        fprintf(2, ".\n");
        worker(p, reader, wp[1]);
    }
    close(wp[1]);
    close(reader);
    fprintf(2, ".\n");
    receiver(wp[0]);
}

int
main(int argc, char *argv[])
{
    int p[2];
    if (pipe(p) < 0) {
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        exit(1);
    }
    if (pid == 0) {
        close(p[0]);
        for (int i = 2; i <= 35; i++) {
            if (write(p[1], &i, sizeof(int)) != sizeof(int)) {
                exit(1);
            }
            // 每写一个数就触发一次 stderr 输出，借 fprintf 内部 uartwrite 的 sleep
            // 来强制调度。在 xv6-riscv (S081 2021) 单核无抢占环境下，这是必要的。
            fprintf(2, "F");
        }
        close(p[1]);
        fprintf(2, ".\n");
        exit(0);
    }
    close(p[1]);
    receiver(p[0]);
    wait(0);
    exit(0);
}
