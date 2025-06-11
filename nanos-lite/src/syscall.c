#include "syscall.h"
#include <common.h>
#include <proc.h>

// 声明yield函数
void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[],
                   char *const envp[]);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;                 // syscall_id
  a[1] = c->GPR2;                 // arg1
  a[2] = c->GPR3;                 // arg2
  a[3] = c->GPR4;                 // arg3
  uintptr_t syscall_id = c->GPR1; // 假设系统调用号存储在GPR1中

  enum {
    SYS_exit,
    SYS_yield,
    SYS_open,
    SYS_read,
    SYS_write,
    SYS_kill,
    SYS_getpid,
    SYS_close,
    SYS_lseek,
    SYS_brk,
    SYS_fstat,
    SYS_time,
    SYS_signal,
    SYS_execve,
    SYS_fork,
    SYS_link,
    SYS_unlink,
    SYS_wait,
    SYS_times,
    SYS_gettimeofday
  };

  printf("System call received: id=%u, args=%u,%u,%u\n",
         (unsigned int)syscall_id, (unsigned int)a[1], (unsigned int)a[2],
         (unsigned int)a[3]);
  switch (syscall_id) {
  case SYS_exit:
    // 处理程序退出系统调用
    printf("Program exit with code %d\n", (int)a[1]);
    // 使用退出状态参数调用halt()
    halt(a[1]);
    break;
  case SYS_yield:
    // 处理yield系统调用
    printf("System call: yield\n");
    // 返回0表示成功
    c->GPRx = 0;
    break;

  case SYS_write:
    // 处理写入系统调用
    if (a[1] == 1 || a[1] == 2) { // fd = 1 (stdout) 或 fd = 2 (stderr)
      // 标准输出或标准错误
      int i;
      char *buf = (char *)a[2];
      int len = a[3];

      for (i = 0; i < len; i++) {
        putch(buf[i]);
      }
      c->GPRx = len; // 返回成功写入的字节数
    } else {
      // 其他文件描述符，暂不支持，返回错误
      printf("Unsupported file descriptor: %d\n", (int)a[1]);
      c->GPRx = -1;
    }
    break;

  case SYS_brk:
    // 内存分配，简单实现：总是成功
    // printf("System call: brk(addr=0x%x)\n", (uint32_t)a[1]);
    c->GPRx = 0; // 返回0表示成功
    break;

  case SYS_open:
  case SYS_read:
  case SYS_close:
  case SYS_lseek:
    // 文件操作，暂时返回错误
    printf("System call: file operation %d (unimplemented)\n", (int)a[0]);
    c->GPRx = -1;
    break;

  case SYS_fstat:
    // 简单实现：返回成功，但不填充stat结构
    printf("System call: fstat(fd=%d, stat_buf=0x%x)\n", (int)a[1], (unsigned int)a[2]);
    // 返回0表示成功
    c->GPRx = 0;
    break;

  case SYS_gettimeofday:
    // 获取系统时间，暂时返回0
    printf("System call: gettimeofday\n");
    c->GPRx = 0;
    break;

  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }
}
