ENTRY(start)

SECTIONS {
    . = 0x1000000;

    .text : {
        KEEP(*(.text.start))
        *(.text .text.*)
    }

    .rodata : ALIGN(4){
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        # 将所有文件的 .bss 段和以 .bss 开头的段放在该位置
        # 同时也包含 .sbss 段和以 .sbss 开头的段
        # .bss 段用于存放未初始化的全局变量和静态变量
        # .sbss 段用于存放较小的未初始化数据
        *(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);
        . += 64 * 1024;
        __stack_top = .;

        // 确保可执行文件不超过 8MB
        ASSERT(. < 0x1800000, "too large executable");
    }
}