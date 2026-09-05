# user/uthread_switch.S
# 用户级线程上下文切换
# RISC-V 汇编实现：保存 callee-saved 寄存器，切换栈
#
# void thread_switch(struct thread *old, struct thread *new);
# a0 = 旧线程指针，a1 = 新线程指针

.text
.globl thread_switch
thread_switch:
    # ===== 保存当前线程的 callee-saved 寄存器 =====
    sd ra, 0(a0)        # 返回地址
    sd sp, 8(a0)        # 栈指针
    sd s0, 16(a0)       # 帧指针
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)     # 最后一个 callee-saved 寄存器

    # ===== 加载新线程的寄存器 =====
    ld ra, 0(a1)        # 恢复返回地址
    ld sp, 8(a1)        # 恢复栈指针
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    ret                  # 跳转到新线程的 ra（可能是函数入口或 thread_schedule 返回点）
