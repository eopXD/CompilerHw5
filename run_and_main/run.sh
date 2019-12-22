#!/bin/bash
$1 $2
/home/ntu/.local/riscv/bin/riscv64-unknown-linux-gnu-gcc -O0 -static main.S
