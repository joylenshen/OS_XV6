# user/thread_switch.S
# RISC-V 汇编：用户级线程上下文切换
# void thread_switch(struct thread *old, struct thread *new);
#
# 保存当前线程的 callee-saved 寄存器到 old，加载 new 的寄存器
# callee-saved: ra, sp, s0-s11
        .text
        .globl thread_switch
        .align 4
thread_switch:
        # 保存当前线程（old）的 callee-saved 寄存器
        sd ra, 0(a0)        # 保存返回地址
        sd sp, 8(a0)        # 保存栈指针
        sd s0, 16(a0)       # s0-s11
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
        sd s11, 104(a0)

        # 加载新线程（new）的 callee-saved 寄存器
        ld ra, 0(a1)
        ld sp, 8(a1)
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

        ret                    # 返回到新线程的调用者（ra 已设为 func 入口或恢复点）
