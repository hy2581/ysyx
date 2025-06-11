#include <common.h>

extern void do_syscall(Context *c);
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
  case EVENT_YIELD: // 自陷事件（yield系统调用）
    printf("Event: yield system call detected(from do event)\n");
    do_syscall(c);
    break;

  case EVENT_ERROR: // 添加对错误事件的处理
    printf("Error event detected, cause = %u(from do event)\n", e.cause);
    do_syscall(c);
    break;

  case EVENT_SYSCALL:
    printf("EVENT_SYSCALL(from do event)\n");
    // 这里可以调用系统调用处理函数
    do_syscall(c);
    break;

  default:
    panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
