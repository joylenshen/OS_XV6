// user/alarmtest.c — MIT 6.S081 Lab4 traps
// 测试 sigalarm: 每 N 个 tick 触发 handler
// Handler 必须调用 sigreturn 恢复执行
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
periodic(void)
{
    printf("alarm!\n");
    sigreturn();   // 恢复原程序
}

// test0: 验证 handler 触发
void
test0(void)
{
    int i;
    printf("test0: starting\n");
    sigalarm(2, periodic);
    for (i = 0; i < 50; i++) {
        printf("iteration %d\n", i);
    }
    sigalarm(0, 0);  // 关闭
    printf("test0: ok\n");
}

void
test1(void)
{
    printf("test1: starting\n");
    sigalarm(2, periodic);
    // 长时间循环
    for (int i = 0; i < 1000 * 1000000; i++) {}
    sigalarm(0, 0);
    printf("test1: ok\n");
}

int
main(int argc, char *argv[])
{
    test0();
    test1();
    printf("alarmtest: all tests passed\n");
    exit(0);
}
