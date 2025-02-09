#include "user.h"

extern char __stack_top[];

__attribute__((noreturn)) void exit(void) {
    while (true);
}

void putchar(char ch) {
    // TODO: 实现 putchar 函数
}

__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    // 设置栈顶指针
    __asm__ __volatile__(
       "mv sp, %[stack_top]\n"
       "call main\n"
       "call exit\n"
       :: [stack_top] "r" (__stack_top)
    );
}