#include "syscall.h"
#include <common.h>
#include <elf.h>
#include <fs.h>
#include <proc.h>
#include <sys/time.h> // 添加这一行，包含timeval结构体定义

// 声明yield函数
void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[],
                   char *const envp[]);

Context *do_syscall(Context *c) {
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

  // printf("System call received: id=%u, args=%u,%u,%u\n",
  //        (unsigned int)syscall_id, (unsigned int)a[1], (unsigned int)a[2],
  //        (unsigned int)a[3]);
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

  case SYS_brk:
    // 打印修改前的 GPRx 值
    c->GPRx = 0;
    // 打印修改后的 GPRx 值
    break;

  case SYS_open:
    c->GPRx = fs_open((const char *)a[1], a[2], a[3]);
    break;

  case SYS_read:
    c->GPRx = fs_read(a[1], (void *)a[2], a[3]);
    break;

  case SYS_write:
    c->GPRx = fs_write(a[1], (const void *)a[2], a[3]);
    break;

  case SYS_lseek:
    c->GPRx = fs_lseek(a[1], a[2], a[3]);
    break;

  case SYS_close:
    c->GPRx = fs_close(a[1]);
    break;

  case SYS_fstat:
    // 简单实现：返回成功，但不填充stat结构
    printf("System call: fstat(fd=%d, stat_buf=0x%x)\n", (int)a[1],
           (unsigned int)a[2]);
    // 返回0表示成功
    c->GPRx = 0;
    break;

  case SYS_gettimeofday: {
    struct timeval *tv = (struct timeval *)a[1];
    // 获取当前时间（从AM中获取）
    if (tv) {
      uint64_t us = io_read(AM_TIMER_UPTIME).us;
      tv->tv_sec = us / 1000000;
      tv->tv_usec = us % 1000000;
    }
    // timezone参数一般忽略
    c->GPRx = 0; // 设置返回值为0表示成功
    break;       // 使用break而不是return
  }

  case SYS_getpid:
    // 简单实现：返回固定的pid值
    printf("System call: getpid\n");
    c->GPRx = 1; // 返回一个固定的进程ID，比如1
    break;

  case SYS_time:
    // 返回一个简单的时间值
    c->GPRx = 0;
    break;

  case SYS_execve:
    // 加载并执行一个新程序
    naive_uload(NULL, (const char *)a[1]);
    c->GPRx = 0; // 成功
    break;

  case SYS_kill:
    // 简单实现：总是返回成功
    printf("System call: kill(pid=%d, sig=%d)\n", (int)a[1], (int)a[2]);
    c->GPRx = 0; // 返回0表示成功
    break;

  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }

  return c; // 返回修改后的上下文
}
