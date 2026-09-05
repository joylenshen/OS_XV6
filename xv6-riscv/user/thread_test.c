// user/thread_test.c
// 测试用户级线程系统
// 演示两个线程交替执行

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void thread_yield(void);

// 测试线程函数
void
thread_func_a(void)
{
    for (int i = 0; i < 5; i++) {
        printf("Thread A: iteration %d\n", i);
        thread_yield();
    }
    printf("Thread A: done\n");
}

void
thread_func_b(void)
{
    for (int i = 0; i < 5; i++) {
        printf("Thread B: iteration %d\n", i);
        thread_yield();
    }
    printf("Thread B: done\n");
}

// 外部线程函数声明
extern void thread_init(void);
extern void thread_create(void (*func)());

int
main(int argc, char *argv[])
{
    printf("Thread test starting...\n");

    // 初始化线程系统
    thread_init();

    // 创建两个线程
    thread_create(thread_func_a);
    thread_create(thread_func_b);

    printf("Main: all threads created, starting scheduler\n");

    // 主调度循环
    // 注意：thread_schedule 会切换到其他线程，
    // 当其他线程退出后，会返回到这里继续循环
    while (1) {
        // 检查是否还有其他可运行的线程
        int has_runnable = 0;
        for (int i = 0; i < 64; i++) {
            extern struct thread all_thread[];
            extern int MAX_THREADS;
            // 简单检查：只要不是 UNUSED 就认为还在运行
            if (i > 0) { // 跳过主线程
                // 这里我们简单处理：只要 yield 回来就继续
            }
        }
        
        // 尝试调度其他线程
        thread_yield();
    }

    printf("Thread test finished.\n");
    exit(0);
}
