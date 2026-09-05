// symlinktest.c - 测试符号链接功能
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(void) {
    char buf[512];
    int fd;

    // 测试 1: 创建普通文件
    fd = open("target.txt", O_CREATE | O_WRONLY);
    if (fd < 0) {
        printf("FAIL: create target.txt\n");
        exit(1);
    }
    write(fd, "hello symlink", 13);
    close(fd);

    // 测试 2: 创建符号链接
    if (symlink("target.txt", "symlink.txt") < 0) {
        printf("FAIL: symlink failed\n");
        exit(1);
    }
    printf("symlink created\n");

    // 测试 3: 通过符号链接读取
    fd = open("symlink.txt", O_RDONLY);
    if (fd < 0) {
        printf("FAIL: open symlink failed\n");
        exit(1);
    }
    int n = read(fd, buf, sizeof(buf));
    if (n < 0 || n != 13) {
        printf("FAIL: read symlink: got %d\n", n);
        exit(1);
    }
    buf[n] = '\0';
    printf("read from symlink: %s\n", buf);
    close(fd);

    // 测试 4: O_NOFOLLOW 标志
    fd = open("symlink.txt", O_RDONLY | O_NOFOLLOW);
    if (fd >= 0) {
        printf("FAIL: O_NOFOLLOW should fail for symlink\n");
        close(fd);
        exit(1);
    }
    printf("O_NOFOLLOW works correctly\n");

    // 清理
    unlink("symlink.txt");
    unlink("target.txt");

    printf("symlinktest: PASS\n");
    exit(0);
}
