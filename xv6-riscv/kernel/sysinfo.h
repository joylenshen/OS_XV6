// Lab syscall: sysinfo 结构体，传递给用户态程序
struct sysinfo {
  uint64 freemem;  // 空闲内存字节数
  uint64 nproc;    // 非 UNUSED 进程数
};
