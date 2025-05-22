# Nanos-lite

Nanos-lite is the simplified version of Nanos (http://cslab.nju.edu.cn/opsystem).
It is ported to the [AM project](https://github.com/NJU-ProjectN/abstract-machine.git).
It is a two-tasking operating system with the following features
* ramdisk device drivers
* ELF program loader
* memory management with paging
* a simple file system
  * with fix number and size of files
  * without directory
  * some device files
* 9 system calls
  * open, read, write, lseek, close, gettimeofday, brk, exit, execve
* scheduler with two tasks


pa3 part1:
1. **mstatus** (0x300)：机器模式状态寄存器
   - 控制全局中断使能
   - 保存异常发生前的特权模式和中断状态

2. **mtvec** (0x305)：机器模式陷阱向量基址
   - 存储异常处理程序的入口地址
   - 在项目中通过`cte_init`函数设置为`__am_asm_trap`

3. **mepc** (0x341)：机器模式异常程序计数器
   - 存储异常发生时的PC值
   - 异常处理返回时加载回PC

4. **mcause** (0x342)：机器模式异常原因
   - 存储导致异常的原因编号
   - 例如：ecall异常为11，断点异常为3


让我们通过`yield()`系统调用来展示整个流程：

1. 应用程序调用`yield()`：
   ```c
   void yield() {
     asm volatile("li a7, -1; ecall");  // 加载a7=-1，执行ecall
   }
   ```

2. `ecall`指令执行，硬件自动：
   - 将PC保存到`mepc` CSR
   - 将11(环境调用异常)写入`mcause` CSR
   - 跳转到`mtvec`指向的地址(`__am_asm_trap`)

3. `__am_asm_trap`保存上下文并调用C处理函数：
   ```assembly
   // 保存寄存器
   csrr t0, mcause    // 读取异常原因
   csrr t1, mstatus   // 读取状态
   csrr t2, mepc      // 读取异常PC

   // 调用C函数处理
   call __am_irq_handle
   ```

4. `__am_irq_handle`处理异常：
   ```c
   // 检查mcause和a7判断是否是yield
   if (c->mcause == 11 && c->gpr[17] == -1) {
     ev.event = EVENT_YIELD;
   }
   // 设置返回地址，跳过ecall指令
   c->mepc += 4;  
   ```

5. 返回到`__am_asm_trap`，恢复上下文：
   ```assembly
   // 恢复修改后的CSR
   csrw mstatus, t1
   csrw mepc, t2      // 加载修改后的mepc

   // 恢复寄存器
   mret               // 返回到mepc指向的地址
   ```

通过这种机制，RISC-V可以高效地实现异常处理和系统调用，并且硬件和软件之间有明确的责任划分，体现了RISC架构的设计哲学。
