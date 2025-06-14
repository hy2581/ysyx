#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

// 函数声明：接收一个指向上下文结构体的指针，并返回同样的指针类型
// Context是包含所有寄存器状态的结构体，由trap.S在栈上构建并传入
Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};


    switch (c->mcause) {
    case 11: // ecall触发的环境调用异常 - 标准RISC-V值为11
// 检查a7/a5寄存器以确定这是否是yield系统调用
// hy:感觉是多费一步劲，在这里设置的event没有意义
#ifdef __riscv_e
      if (c->gpr[15] == (uintptr_t)-1) { // a5寄存器用于RV32E
#else
      if (c->gpr[17] == (uintptr_t)-1) { // a7寄存器用于标准RISC-V
#endif
        ev.event = EVENT_YIELD;
        // printf("Debug: YIELD系统调用已识别\n");
      } else {
        ev.event = EVENT_SYSCALL;
        //         printf("Debug: 其他系统调用，syscall number = %u\n",
        // #ifdef __riscv_e
        //                c->gpr[15]
        // #else
        //                c->gpr[17]
        // #endif
        //         );
      }
      break;

    default:
      ev.event = EVENT_ERROR;
      ev.cause = c->mcause;
      printf("Debug: 未知异常，mcause = %u\n", c->mcause);
      break;
    }
    //将执行do_event
    c = user_handler(ev, c);
    assert(c != NULL);
  }

  // 只有对ecall指令才需要加4，其他异常可能不需要
  if (c->mcause == 11) {
    c->mepc = c->mepc + 4;
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
