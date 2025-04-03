# ICS2024 Programming Assignment

This project is the programming assignment of the class ICS(Introduction to Computer System)
in Department of Computer Science and Technology, Nanjing University.

For the guide of this programming assignment,
refer to https://nju-projectn.github.io/ics-pa-gitbook/ics2024/

To initialize, run
```bash
bash init.sh subproject-name
```
See `init.sh` for more details.

The following subprojects/components are included. Some of them are not fully implemented.
* [NEMU](https://github.com/NJU-ProjectN/nemu)
* [Abstract-Machine](https://github.com/NJU-ProjectN/abstract-machine)
* [Nanos-lite](https://github.com/NJU-ProjectN/nanos-lite)
* [Navy-apps](https://github.com/NJU-ProjectN/navy-apps)

可能用到的指令：
am-kernels/tests/cpu-tests/里的make ARCH=riscv32-nemu ALL=dummy run
am-kernels/tests/cpu-tests/  
make ARCH=riscv32-nemu ALL=hello-str run
/home/hy258/ics2024/nemu
./build/riscv32-nemu-interpreter --elf /home/hy258/ics2024/am-kernels/tests/cpu-tests/build/add-longlong-riscv32-nemu.elf /home/hy258/ics2024/am-kernels/tests/cpu-tests/build/add-longlong-riscv32-nemu.bin

