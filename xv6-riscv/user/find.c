// user/find.c — MIT 6.S081 Lab1 util
// 实验目的：实现简化版 find，在目录树中按文件名查找
// 参考实现思路：递归遍历目录，对每项调用 fstat 判别
//   - 普通文件：比较文件名，匹配则打印完整路径
//   - 目录：递归进入（注意 . 和 ..）
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

// 字符串匹配（不依赖 libc strcmp）
int
matchhere(char *name, char *pattern)
{
    if (*pattern == '\0')
        return 1;
    if (*pattern == '*') {
        // 跳过连续的 *
        while (*pattern == '*')
            pattern++;
        if (*pattern == '\0')
            return 1;
        // 尝试在 name 中逐位置匹配
        for (; *name; name++)
            if (matchhere(name, pattern))
                return 1;
        return 0;
    }
    if (*name != *pattern)
        return 0;
    return matchhere(name + 1, pattern + 1);
}

int
match(char *name, char *pattern)
{
    if (matchhere(name, pattern))
        return 1;
    return 0;
}

void
find(char *path, char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
    case T_FILE:
        // 提取文件名（最后一段）并匹配
        {
            char *fname = path;
            for (char *q = path; *q; q++) {
                if (*q == '/' && *(q + 1) != '\0')
                    fname = q + 1;
            }
            if (match(fname, target))
                printf("%s\n", path);
        }
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
            fprintf(2, "find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            find(buf, target);
        }
        break;
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(2, "usage: find PATH NAME\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
