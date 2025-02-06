#include "common.h"

void putchar(char ch);

void printf(const char* fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case '\0':
                    putchar('%');
                    goto end;
                case '%':
                    putchar('%');
                    break;
                case 's': {
                    const char* s = va_arg(vargs, const char*);
                    while (*s) {
                        putchar(*s++);
                    }
                    break;
                }
                case 'd': {
                    int value = va_arg(vargs, int);
                    if (value < 0) {
                        putchar('-');
                        value = -value;
                    }
                    // 将value转换为字符串
                    int divisor = 1;
                    while (value / divisor > 9) divisor *= 10;
                    // 从最高位到最低位，每次处理1个数字
                    while (divisor > 0) {
                        putchar('0' + value / divisor);
                        value %= divisor;
                        divisor /= 10;
                    }
                    break;
                }
                case 'x': {
                    // 32-bit integer to hex string
                    // 从最高位到最低位，每次处理4个bit
                    int value = va_arg(vargs, int);
                    for (int i = 7; i >= 0; i--) {
                        int nibble = (value >> (i * 4)) & 0xf;
                        // nibble是索引，"0123456789abcdef"是字符串，nibble作为索引，取出一个字符
                        putchar("0123456789abcdef"[nibble]);
                    }
                }
            }
        } else {
            putchar(*fmt);
        }
        fmt++;
    }

end:
    va_end(vargs);
}