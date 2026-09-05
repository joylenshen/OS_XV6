// user/user.h — 用户态头文件

// 系统调用错误返回值
#define SBRK_ERROR ((char *)-1)

struct stat;
struct sysinfo {
  uint64 freemem;
  uint64 nproc;
};

// Lab mmap: mmap 用的保护标志和映射类型
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_SHARED  0x1
#define MAP_PRIVATE 0x2

// Lab pgtbl: usyscall page layout (shared with kernel/memlayout.h)
struct usyscall {
  int pid;
};

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int *);
int pipe(int *);
int write(int, const void *, int);
int read(int, void *, int);
int close(int);
int kill(int);
int exec(const char *, char **);
int open(const char *, int);
int mknod(const char *, short, short);
int unlink(const char *);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int mkdir(const char *);
int chdir(const char *);
int dup(int);
char *sys_sbrk(int, int);
char *sbrk(int);
char *sbrklazy(int);
int pause(int);
int sleep(int);
int uptime(void);
int sync(void);

// Lab syscall additions
int trace(int);
int sysinfo(struct sysinfo *);

// Lab traps additions
int sigalarm(int ticks, void (*handler)());
int sigreturn(void);

// Lab pgtbl additions
int pgaccess(void *base, int len, void *maskbuf);

// Lab fs additions
int symlink(char *target, char *path);

// Lab mmap additions
uint64 mmap(void *addr, int length, int prot, int flags, int fd, int offset);
int munmap(void *addr, int length);

// Lab pgtbl: 加速 getpid - 直接读 USYSCALL 页（不走 syscall）
// 由 user/getpid.c 提供实现
int getpid(void);

// ulib.c
int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int strcmp(const char *, const char *);
void fprintf(int, const char *, ...);
void printf(const char *, ...);
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);
void *malloc(uint);
void free(void *);
int atoi(const char *);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
