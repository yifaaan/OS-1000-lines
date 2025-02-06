#!/bin/bash
set -xue

# QEMU 文件路径
QEMU=qemu-system-riscv32

# 启动 QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot