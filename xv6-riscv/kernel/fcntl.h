#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR   0x002
#define O_CREATE 0x200
#define O_TRUNC  0x400
#define O_NOFOLLOW 0x004

// Lab mmap: 保护标志和映射类型（与 user/user.h 一致）
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_SHARED  0x1
#define MAP_PRIVATE 0x2
