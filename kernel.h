#pragma once

#include "common.h"

#define PANIC(fmt, ...)                                                        \
    do {                                                                         \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
        while (1) {}                                                               \
    } while (0)

struct sbiret {
    long error;
    long value;
};



struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));

#define READ_CSR(reg) \
    ({ \
        unsigned long __tmp; \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp)); \
        __tmp; \
    })

#define WRITE_CSR(reg, value) \
    do { \
        uint32_t __tmp = (value); \
        __asm__ __volatile__("csrw " #reg ", %0" :: "r"(__tmp)); \
    } while (0)


// 进程最大数量
#define PROCS_MAX 8
// 未使用的进程
#define PROC_UNUSED 0
// 可运行进程
#define PROC_RUNNABLE 1

struct process {
    int pid;
    int state;
    vaddr_t sp;
    // 页表
    uint32_t* page_table;
    // 内核栈
    uint8_t stack[8192];
};

// 页表模式, single bit in the satp register
#define SATP_SV32 (1u << 31)
// 页表是否有效
#define PAGE_V (1 << 0)
// 读权限
#define PAGE_R (1 << 1)
// 写权限
#define PAGE_W (1 << 2)
// 执行权限
#define PAGE_X (1 << 3)
// 用户权限
#define PAGE_U (1 << 4)


#define USER_BASE 0x1000000

#define SSTATUS_SPIE (1 << 5)