// user/bigfile.c — MIT 6.S081 Lab9 fs: 大文件测试
// 测试双重间接块支持的最大文件大小
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int fd;
    char buf[1024];
    memset(buf, 'x', sizeof(buf));

    // 写一个 ~1MB 的文件（远大于原版 552KB 限制）
    // 注意：文件名用 bigfile_data 而非 bigfile，避免与 _bigfile 程序同 inode 名冲突
    fd = open("bigfile_data", O_CREATE | O_WRONLY);
    if (fd < 0) {
        printf("bigfile: open failed\n");
        exit(1);
    }
    for (int i = 0; i < 1024; i++) {
        if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
            printf("bigfile: write failed at block %d\n", i);
            exit(1);
        }
    }
    close(fd);

    // 读取并验证
    fd = open("bigfile_data", O_RDONLY);
    if (fd < 0) {
        printf("bigfile: reopen failed\n");
        exit(1);
    }
    int total = 0;
    while (1) {
        int n = read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        total += n;
    }
    close(fd);
    unlink("bigfile_data");
    printf("bigfile: wrote %d bytes\n", total);
    if (total == 1024 * 1024) {
        printf("bigfile: OK\n");
        exit(0);
    }
    printf("bigfile: FAIL\n");
    exit(1);
}
