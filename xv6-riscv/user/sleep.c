// user/sleep.c — MIT 6.S081 Lab1 util
// 实验目的：实现 xv6 的 sleep 用户程序
// 参考实现：根据命令行参数暂停若干 ticks（时钟中断周期）
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "usage: sleep ticks\n");
        exit(1);
    }
    int ticks = atoi(argv[1]);
    if (ticks < 0) {
        fprintf(2, "sleep: invalid tick count %s\n", argv[1]);
        exit(1);
    }
    // 调用内核态 sleep 系统调用，由 xv6 内核完成等待
    sleep(ticks);
    exit(0);
}
