// user/trace.c — MIT 6.S081 Lab syscall
// 用法: trace mask program [args...]
// 将 mask 对应的系统调用执行时打印 trace 信息
// mask 为按位掩码，如 32 = 1<<SYS_read, 2147483647 = 所有 31 个低 bit
#include "kernel/types.h"
#include "kernel/syscall.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(2, "usage: trace mask command [args...]\n");
        exit(1);
    }
    int mask = atoi(argv[1]);
    trace(mask);
    exec(argv[2], &argv[2]);
    fprintf(2, "trace: exec %s failed\n", argv[2]);
    exit(1);
}
