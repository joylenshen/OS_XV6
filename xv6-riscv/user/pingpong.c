// user/pingpong.c — MIT 6.S081 Lab1 util
// 实验目的：演示进程间通过一对管道进行双向通信（ping-pong）
// 实现思路：
//   1. 创建两个管道 p2c (parent->child) 和 c2p (child->parent)
//   2. 父进程 fork 出子进程
//   3. 父写 ping 到 p2c，子读 ping 并写 pong 到 c2p
//   4. 父从 c2p 读 pong
//
// 关键点：xv6-riscv (S081 2021) 单核调度器在 pipe wakeup 后未必立刻切换，
// 需要用 fprintf(2,...) 主动调用 uartwrite → sleep 来强制 yield。
// 同 primes 修复方案。
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p2c[2], c2p[2];
    char buf[1] = {'X'};
    int pid;

    if (pipe(p2c) < 0 || pipe(c2p) < 0) {
        fprintf(2, "pingpong: pipe failed\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        fprintf(2, "pingpong: fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        // 子进程：从 p2c 读，再向 c2p 写
        close(p2c[1]);   // 关闭 p2c 的写端
        fprintf(2, ".");  // yield: 让父执行 write

        if (read(p2c[0], buf, 1) != 1) {
            fprintf(2, "pingpong: child read failed\n");
            exit(1);
        }
        close(p2c[0]);

        printf("%d: received ping\n", getpid());

        if (write(c2p[1], buf, 1) != 1) {
            fprintf(2, "pingpong: child write failed\n");
            exit(1);
        }
        close(c2p[1]);
        // 主动 yield，让父进程被调度
        fprintf(2, ".");
        exit(0);
    }

    // 父进程
    close(p2c[0]);   // 关闭 p2c 的读端
    close(c2p[1]);   // 关闭 c2p 的写端

    if (write(p2c[1], buf, 1) != 1) {
        fprintf(2, "pingpong: parent write failed\n");
        exit(1);
    }
    // 写完后立即关闭 p2c[1]，让子进程能读到字节
    close(p2c[1]);
    // 主动 yield，让子进程执行 read + write
    fprintf(2, ".");

    if (read(c2p[0], buf, 1) != 1) {
        fprintf(2, "pingpong: parent read failed\n");
        exit(1);
    }
    close(c2p[0]);

    wait(0);
    printf("%d: received pong\n", getpid());
    exit(0);
}
