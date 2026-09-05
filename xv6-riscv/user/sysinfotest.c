// user/sysinfotest.c — MIT 6.S081 Lab syscall
// 测试 sysinfo 系统调用：验证 freemem > 0 且 nproc >= 1
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    struct sysinfo info;
    if (sysinfo(&info) < 0) {
        printf("sysinfotest: sysinfo failed\n");
        exit(1);
    }
    printf("sysinfotest: freemem=%lu nproc=%lu\n", info.freemem, info.nproc);
    if (info.freemem > 0 && info.nproc >= 1) {
        printf("sysinfotest: OK\n");
        exit(0);
    }
    printf("sysinfotest: FAIL\n");
    exit(1);
}
