#include "kernel.h"
#include "common.h"


// 导入kernel链接器中的符号
extern char __bss[], __bss_end[], __stack_top[];

// 导入shell.bin.o链接器中的符号
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

struct process procs[PROCS_MAX];
extern char __kernel_base[];



// 当前进程
struct process* current_proc;
// 空闲进程
struct process* idle_proc;






/// 内核入口
/// 这个函数是内核的入口，它将栈指针设置为sscratch寄存器中的值，并保存当前的寄存器值到栈中
/// 然后跳转到kernel_main函数执行
__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
        "csrrw sp, sscratch, sp\n" 
        "addi sp, sp, -4 * 31\n" // 将sp的值减去4 * 31, 为保存寄存器值准备空间
        "sw ra,  4 * 0(sp)\n" // 保存返回地址寄存器
        "sw gp,  4 * 1(sp)\n" // 保存全局指针
        "sw tp,  4 * 2(sp)\n" // 保存线程指针
        "sw t0,  4 * 3(sp)\n" // 将t0的值保存到sp的3位置
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        // 保存发生异常的sp
        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n" 

        // sscrach存储新的内核栈sp
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        "mv a0, sp\n" // 将sp的值保存到a0寄存器中, 用于传递给handle_trap函数
        "call handle_trap\n" // 调用handle_trap函数

        "lw ra,  4 * 0(sp)\n" // 从sp的0位置恢复返回地址寄存器
        "lw gp,  4 * 1(sp)\n" // 从sp的1位置恢复全局指针
        "lw tp,  4 * 2(sp)\n" // 从sp的2位置恢复线程指针
        "lw t0,  4 * 3(sp)\n" // 从sp的3位置恢复t0寄存器
        "lw t1,  4 * 4(sp)\n" // 从sp的4位置恢复t1寄存器
        "lw t2,  4 * 5(sp)\n" // 从sp的5位置恢复t2寄存器
        "lw t3,  4 * 6(sp)\n" // 从sp的6位置恢复t3寄存器
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}

extern char __free_ram[], __free_ram_end[];

// 分配n个物理页
paddr_t alloc_pages(uint32_t n) {
    // 静态变量，用于存储下一个可用的物理页地址, 初始化为__free_ram
    static paddr_t next_paddr = (paddr_t)__free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t)__free_ram_end) {
        PANIC("out of memory");
    }
    memset((void*)paddr, 0, n * PAGE_SIZE);
    return paddr;
}


__attribute__((naked)) 
void switch_context(uint32_t* prev_sp, uint32_t* next_sp) {
    __asm__ __volatile__(
        // Save callee-saved registers onto the current process's stack.
        "addi sp, sp, -13 * 4\n" // Allocate stack space for 13 4-byte registers
        "sw ra,  0  * 4(sp)\n"   // Save callee-saved registers only
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // Switch the stack pointer.
        "sw sp, (a0)\n"         // *prev_sp = sp;
        "lw sp, (a1)\n"         // Switch stack pointer (sp) here

        // Restore callee-saved registers from the next process's stack.
        "lw ra,  0  * 4(sp)\n"  // Restore callee-saved registers only
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"  // We've popped 13 4-byte registers from the stack
        "ret\n"
    );
}



struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                        : "=r"(a0), "=r"(a1)
                        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                        : "memory");

    return (struct sbiret){a0, a1};
} 

void yield(void) {
    // 找到下一个可运行的进程
    struct process* next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process* proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    if (next == current_proc) return;

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r" (SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)),
        [sscratch] "r" ((uint32_t)&next->stack[sizeof(next->stack)])
    );
    struct process* prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &current_proc->sp);
}


void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.value;
}


void handle_syscall(struct trap_frame* tf) {
    switch (tf->a3) {
        case SYS_PUTCHAR:
            putchar(tf->a0);
            break;
        case SYS_GETCHAR:
            while (true) {
                long ch = getchar();
                if (ch >= 0) {
                    tf->a0 = ch;
                    break;
                }
                yield();
            }
            break;
        default:
            PANIC("a0 = %x, a1 = %x, a2 = %x, a3 = %x, a4 = %x, a5 = %x, a6 = %x, a7 = %x\n", tf->a0, tf->a1, tf->a2, tf->a3, tf->a4, tf->a5, tf->a6, tf->a7);
            PANIC("unexpected syscall a3 = %x\n", tf->a3);
    }
}

/// 异常处理程序保存CSR寄存器
void handle_trap(struct trap_frame* tf) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    // 存储发生异常的指令的地址
    uint32_t user_pc = READ_CSR(sepc);

    if (scause == SCAUSE_ECALL) {
        // 系统调用
        handle_syscall(tf);
        user_pc += 4;
    } else if (scause == 0xf) {
        printf("Store page fault at address %x\n", stval);
        printf("Current process: pid=%d\n", current_proc->pid);
        printf("Fault address details:\n");
        printf("  - Base offset from USER_BASE: %x\n", stval - USER_BASE);
        printf("  - Page number: %x\n", stval >> 12);
        
        // 检查页表项
        uint32_t vpn1 = (stval >> 22) & 0x3ff;
        uint32_t vpn0 = (stval >> 12) & 0x3ff;
        uint32_t* table1 = current_proc->page_table;
        
        printf("Page table details:\n");
        printf("  - VPN1: %x\n", vpn1);
        printf("  - VPN0: %x\n", vpn0);
        printf("  - Level 1 PTE: %x\n", table1[vpn1]);
        
        if ((table1[vpn1] & PAGE_V) == 0) {
            printf("Level 1 page table entry is invalid\n");
        } else {
            uint32_t* table0 = (uint32_t*)((table1[vpn1] >> 10) * PAGE_SIZE);
            uint32_t pte = table0[vpn0];
            printf("  - Level 0 PTE: %x\n", pte);
        }

        // 打印用户程序的映射信息
        printf("Process memory mapping:\n");
        printf("  - Program start: %x\n", USER_BASE);
        printf("  - Stack top: %x\n", 0x1800000);
        PANIC("Store page fault");  
    } else {
        PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }
    WRITE_CSR(sepc, user_pc);
}

/// 映射页表，将虚拟地址vaddr映射到物理地址paddr，并设置flags
/// https://danielmangum.com/risc-v-tips/2021-11-27-sv-32-address-translation/
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);
    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    // 如果页表项无效，则创建一个页表项
    if ((table1[vpn1] & PAGE_V) == 0) {
        // 创建二级页表
        uint32_t pt_paddr = alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t* table0 = (uint32_t*)((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}




__attribute__((naked))
void user_entry(void) {
    __asm__ __volatile__(
        "csrw sepc, %[sepc]\n"
        "csrw sstatus, %[sstatus]\n"
        "sret\n"
        :
        : [sepc] "r" (USER_BASE),
          [sstatus] "r" (SSTATUS_SPIE)
    );
}

/// 创建一个进程
/// @param image 进程的二进制文件
/// @param image_size 进程的二进制文件大小
struct process* create_process(const void* image, size_t image_size) {
    // 遍历进程控制块数组，找到一个未使用的进程控制块
    struct process* proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }
    if (!proc) PANIC("no free process slots");

    uint32_t* sp = (uint32_t*)&proc->stack[sizeof(proc->stack)];
    // s11 = sp
    *--sp = 0;
    // s10 = 0
    *--sp = 0;
    // s9 = 0
    *--sp = 0;
    // s8 = 0
    *--sp = 0;
    // s7 = 0
    *--sp = 0;
    // s6 = 0
    *--sp = 0;
    // s5 = 0
    *--sp = 0;
    // s4 = 0
    *--sp = 0;
    // s3 = 0
    *--sp = 0;
    // s2 = 0
    *--sp = 0;
    // s1 = 0
    *--sp = 0;
    // s0 = 0
    *--sp = 0;
    // ra = pc
    *--sp = (uint32_t)user_entry;

    uint32_t* page_table = (uint32_t*)alloc_pages(1);
    for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end; paddr += PAGE_SIZE) {
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    }

    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);
        // 最后一段数据可能会小于PAGE_SIZE
        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

        memcpy((void*)page, (void*)(image + off), copy_size);
        map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
        printf("Mapped program page: vaddr=%x, paddr=%x, size=%x\n", 
               USER_BASE + off, page, copy_size);
    }


    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t)sp;
    proc->page_table = page_table;
    return proc;
}


void delay(void) {
    for (int i = 0; i < 30000000; i++) {
        __asm__ __volatile__("nop");
    }
}





struct process* proc_a;
struct process* proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (true) {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (true) {
        putchar('B');
        yield();
    }
}




void kernel_main(void) {
    memset(__bss, 0, __bss_end - __bss);
    
    // 设置内核的入口地址
    WRITE_CSR(stvec, (uint32_t)kernel_entry);
    // __asm__ __volatile__("unimp");
    // PANIC("booted!");
    // printf("unreachable here!\n");

    idle_proc = create_process(NULL, 0);
    idle_proc->pid = -1;
    current_proc = idle_proc;
    create_process(_binary_shell_bin_start, (size_t)_binary_shell_bin_size);
    yield();
    PANIC("switched to idle process!");
}


__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // 设置sp指向连接器设置的栈结束位置
        "j kernel_main\n" // 跳转到内核入口
        :
        : [stack_top] "r" (__stack_top)
    );
}