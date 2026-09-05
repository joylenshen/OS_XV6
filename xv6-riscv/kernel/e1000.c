// e1000.c - Intel E1000 网卡驱动
// 简化版本：仅用于代码框架讲解，实际运行需要 QEMU 模拟或真实硬件

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// E1000 寄存器偏移量
#define E1000_TDBAL   0x03800  // TX Descriptor Base Address Low
#define E1000_TDBAH   0x03804  // TX Descriptor Base Address High
#define E1000_TDLEN   0x03808  // TX Descriptor Length
#define E1000_TDH     0x03810  // TX Descriptor Head
#define E1000_TDT     0x03818  // TX Descriptor Tail
#define E1000_TCTL    0x00400  // TX Control
#define E1000_TIPG    0x00410  // TX Inter-packet Gap

#define E1000_RDBAL   0x02800  // RX Descriptor Base Address Low
#define E1000_RDBAH   0x02804  // RX Descriptor Base Address High
#define E1000_RDLEN   0x02808  // RX Descriptor Length
#define E1000_RDH     0x02810  // RX Descriptor Head
#define E1000_RDT     0x02818  // RX Descriptor Tail
#define E1000_RCTL    0x00100  // RX Control

#define E1000_RA      0x05400  // Receive Address (MAC)

// TX/RX Ring 大小
#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

// 描述符状态位
#define E1000_TXD_CMD_EOP  0x01  // End of Packet
#define E1000_TXD_CMD_IFCS 0x02  // Insert FCS (Frame Check Sequence)
#define E1000_TXD_CMD_RS   0x08  // Report Status
#define E1000_TXD_STAT_DD   0x01  // Descriptor Done

#define E1000_RXD_STAT_DD  0x01  // Descriptor Done
#define E1000_RXD_STAT_EOP 0x02  // End of Packet

// TX 描述符结构
struct tx_desc {
    volatile uint64 addr;      // 数据缓冲区物理地址
    volatile uint16 length;     // 数据长度
    volatile uint16 cso;        // Checksum Offset
    volatile uint8  cmd;        // 命令 (EOP, IFCS, RS)
    volatile uint8  status;     // 状态 (DD)
    volatile uint8  css;        // Checksum Start
    volatile uint8  special;    // 特殊字段
};

// RX 描述符结构
struct rx_desc {
    volatile uint64 addr;       // 数据缓冲区物理地址
    volatile uint16 length;     // 接收到的数据长度
    volatile uint16 checksum;   // 校验和
    volatile uint8  status;     // 状态
    volatile uint8  errors;     // 错误
    volatile uint16 special;    // 特殊字段
};

// 发送/接收环
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));

// 缓冲区（实际应该从内存池分配）
static char tx_bufs[TX_RING_SIZE][2048];
static char rx_bufs[RX_RING_SIZE][2048];

// MMIO 基址
static volatile uint32 *regs;

// 环索引
static int tx_tail = 0;
static int rx_tail = 0;

// 锁（用于多核保护）
static struct spinlock e1000_lock;

// MAC 地址
static uint8 e1000_mac[6];

void
e1000_init(uint64 mmio_base)
{
    initlock(&e1000_lock, "e1000");

    regs = (volatile uint32 *)mmio_base;

    // 读取 MAC 地址
    for (int i = 0; i < 6; i++) {
        e1000_mac[i] = (uint8)((regs[(E1000_RA/4) + i/2] >> (8 * (i%2))) & 0xFF);
    }
    printk("e1000: MAC address %x:%x:%x:%x:%x:%x\n",
           e1000_mac[0], e1000_mac[1], e1000_mac[2],
           e1000_mac[3], e1000_mac[4], e1000_mac[5]);

    // 初始化 TX 环
    memset(tx_ring, 0, sizeof(tx_ring));
    for (int i = 0; i < TX_RING_SIZE; i++) {
        tx_ring[i].addr = (uint64)tx_bufs[i];
        tx_ring[i].status = E1000_TXD_STAT_DD;  // 初始化为完成状态
    }

    // 设置 TX 寄存器
    regs[E1000_TDBAL/4] = (uint32)((uint64)tx_ring);
    regs[E1000_TDBAH/4] = (uint32)((uint64)tx_ring >> 32);
    regs[E1000_TDLEN/4] = TX_RING_SIZE * sizeof(struct tx_desc);
    regs[E1000_TDH/4] = 0;
    regs[E1000_TDT/4] = 0;

    // 设置 TX 控制寄存器：启用发送
    regs[E1000_TCTL/4] = 0x00040100;  // Enable + pre-normalized priority
    regs[E1000_TIPG/4] = 0x00602008;   // 标准 IPG 值

    // 初始化 RX 环
    memset(rx_ring, 0, sizeof(rx_ring));
    for (int i = 0; i < RX_RING_SIZE; i++) {
        rx_ring[i].addr = (uint64)rx_bufs[i];
        rx_ring[i].status = 0;
    }

    // 设置 RX 寄存器
    regs[E1000_RDBAL/4] = (uint32)((uint64)rx_ring);
    regs[E1000_RDBAH/4] = (uint32)((uint64)rx_ring >> 32);
    regs[E1000_RDLEN/4] = RX_RING_SIZE * sizeof(struct rx_desc);
    regs[E1000_RDH/4] = 0;
    regs[E1000_RDT/4] = RX_RING_SIZE - 1;

    // 设置 RX 控制寄存器：启用接收
    regs[E1000_RCTL/4] = 0x00000002;  // Enable receiver

    tx_tail = 0;
    rx_tail = 0;

    printk("e1000: initialized\n");
}

// 发送数据包
// 返回 0 表示成功，-1 表示队列已满
int
e1000_transmit(char *buf, int len)
{
    acquire(&e1000_lock);

    struct tx_desc *desc = &tx_ring[tx_tail];

    // 检查 DD 位：上一个包是否已发送完毕
    if ((desc->status & E1000_TXD_STAT_DD) == 0) {
        release(&e1000_lock);
        return -1;  // 队列已满
    }

    // 复制数据到缓冲区
    if (len > 2048) len = 2048;
    memmove((char *)desc->addr, buf, len);

    // 设置描述符
    desc->length = len;
    desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    desc->status = 0;
    desc->cso = 0;

    // 移动尾指针并通知网卡
    tx_tail = (tx_tail + 1) % TX_RING_SIZE;
    regs[E1000_TDT/4] = tx_tail;

    release(&e1000_lock);
    return 0;
}

// 接收数据包
// 返回接收的字节数，-1 表示没有数据
int
e1000_recv(char *buf, int maxlen)
{
    acquire(&e1000_lock);

    struct rx_desc *desc = &rx_ring[rx_tail];

    // 检查 DD 位：是否有新数据
    if ((desc->status & E1000_RXD_STAT_DD) == 0) {
        release(&e1000_lock);
        return -1;  // 没有数据
    }

    // 获取数据长度
    int len = desc->length;
    if (len > maxlen) len = maxlen;

    // 复制数据
    memmove(buf, (char *)desc->addr, len);

    // 清空状态，准备下一次接收
    desc->status = 0;

    // 通知网卡这个描述符可用
    rx_tail = (rx_tail + 1) % RX_RING_SIZE;
    regs[E1000_RDT/4] = rx_tail;

    release(&e1000_lock);
    return len;
}

// 获取 MAC 地址
void
e1000_get_mac(uint8 *mac)
{
    for (int i = 0; i < 6; i++)
        mac[i] = e1000_mac[i];
}
