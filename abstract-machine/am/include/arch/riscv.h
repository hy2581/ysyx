#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

struct Context {
  // 按照trap.S中保存的顺序排列
  uintptr_t gpr[NR_REGS];  // 首先保存通用寄存器
  uintptr_t mcause;        // 然后是mcause
  uintptr_t mstatus;       // 接着是mstatus
  uintptr_t mepc;          // 最后是mepc
  void *pdir;              // 地址空间信息
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[10]
#define GPR3 gpr[11]
#define GPR4 gpr[12]
#define GPRx gpr[10]

#endif
