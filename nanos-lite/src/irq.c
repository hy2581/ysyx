#include <common.h>

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
  case EVENT_YIELD: // 自陷事件（yield系统调用）
    printf("Event: yield system call detected\n");
    break;

  case EVENT_ERROR: // 添加对错误事件的处理
    printf("Error event detected, cause = %u\n", e.cause);
    break;

    // 如果你有其他需要处理的事件，可以在这里添加

  default:
    panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
