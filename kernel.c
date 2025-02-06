typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// 导入链接器中的符号
extern char __bss[], __bss_end[], __stack_top[];

void* memset(void* buf, char c, size_t n) {
    uint8_t* p = (uint8_t*) buf;
    while (n--) *p++ = c;
    return buf;
}


void kernel_main(void) {
    memset(__bss, 0, __bss_end - __bss);

    for (;;);
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