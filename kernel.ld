# boot 函数是链接器入口，链接器会从 boot 函数开始链接
ENTRY(boot)

SECTIONS {
    # . 表示当前地址，随着链接器不断添加内容，. 会不断增加
    . = 0x80200000;

    .text :{
        # 特别保留 boot 函数，确保它不会被优化掉
        KEEP(*(.text.boot));
        # 将所有文件的 .text 段和以 .text 开头的段放在该位置
        *(.text .text.*);
    }

    # 只读数据段
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    # 数据段
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    # 未初始化数据段
    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    # 栈底
    . = ALIGN(4);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;
}