// user/xargs.c — MIT 6.S081 Lab1 util
// 实验目的：实现简化版 xargs，从 stdin 读行，每行作为参数执行指定命令
// 实现思路：
//   - 把 argv[1..] 作为初始参数
//   - 从 stdin 一行一行读，每行作为最后一个参数追加
//   - fork + exec 执行
//   - 读完后执行最后一行
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXSZ 512

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "usage: xargs command [args...]\n");
        exit(1);
    }

    char line[MAXSZ];
    char *new_argv[MAXARG];
    int n;

    // 复制初始参数
    for (n = 0; n < argc - 1; n++)
        new_argv[n] = argv[n + 1];
    new_argv[argc - 1] = line;
    new_argv[argc] = 0;

    int i = 0;
    char c;
    while (read(0, &c, 1) == 1) {
        if (c == '\n') {
            line[i] = 0;
            if (i == 0)
                continue;       // 跳过空行
            if (fork() == 0) {
                exec(new_argv[0], new_argv);
                fprintf(2, "xargs: exec %s failed\n", new_argv[0]);
                exit(1);
            }
            wait(0);
            i = 0;
        } else if (i < MAXSZ - 1) {
            line[i++] = c;
        } else {
            // 单行超长：截断后处理
            line[i] = 0;
            if (fork() == 0) {
                exec(new_argv[0], new_argv);
                fprintf(2, "xargs: exec %s failed\n", new_argv[0]);
                exit(1);
            }
            wait(0);
            i = 0;
        }
    }
    // 处理最后一行（如果文件不以 \n 结尾）
    if (i > 0) {
        line[i] = 0;
        if (fork() == 0) {
            exec(new_argv[0], new_argv);
            fprintf(2, "xargs: exec %s failed\n", new_argv[0]);
            exit(1);
        }
        wait(0);
    }
    exit(0);
}
