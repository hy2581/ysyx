#include <proc.h>

#define MAX_NR_PROC 4

extern void naive_uload(PCB *pcb, const char *filename);

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {
  switch_boot_pcb();
  Log("Initializing processes...");

  // 使用正确的ramdisk路径
  naive_uload(NULL, "/bin/bird");
  // 或者
  // naive_uload(NULL, "/bin/hello");

  Log("Processes initialized.");
}

Context* schedule(Context *prev) {
  return NULL;
}
