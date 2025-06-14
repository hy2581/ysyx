#include "syscall.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// helper macros
#define _concat(x, y) x ## y
#define concat(x, y) _concat(x, y)
#define _args(n, list) concat(_arg, n) list
#define _arg0(a0, ...) a0
#define _arg1(a0, a1, ...) a1
#define _arg2(a0, a1, a2, ...) a2
#define _arg3(a0, a1, a2, a3, ...) a3
#define _arg4(a0, a1, a2, a3, a4, ...) a4
#define _arg5(a0, a1, a2, a3, a4, a5, ...) a5

// extract an argument from the macro array
#define SYSCALL  _args(0, ARGS_ARRAY)
#define GPR1 _args(1, ARGS_ARRAY)
#define GPR2 _args(2, ARGS_ARRAY)
#define GPR3 _args(3, ARGS_ARRAY)
#define GPR4 _args(4, ARGS_ARRAY)
#define GPRx _args(5, ARGS_ARRAY)

// ISA-depedent definitions
#if defined(__ISA_X86__)
# define ARGS_ARRAY ("int $0x80", "eax", "ebx", "ecx", "edx", "eax")
#elif defined(__ISA_MIPS32__)
# define ARGS_ARRAY ("syscall", "v0", "a0", "a1", "a2", "v0")
#elif defined(__riscv)
#ifdef __riscv_e
# define ARGS_ARRAY ("ecall", "a5", "a0", "a1", "a2", "a0")
#else
# define ARGS_ARRAY ("ecall", "a7", "a0", "a1", "a2", "a0")
#endif
#elif defined(__ISA_AM_NATIVE__)
# define ARGS_ARRAY ("call *0x100000", "rdi", "rsi", "rdx", "rcx", "rax")
#elif defined(__ISA_X86_64__)
# define ARGS_ARRAY ("int $0x80", "rdi", "rsi", "rdx", "rcx", "rax")
#elif defined(__ISA_LOONGARCH32R__)
# define ARGS_ARRAY ("syscall 0", "a7", "a0", "a1", "a2", "a0")
#else
#error _syscall_ is not implemented
#endif

intptr_t _syscall_(intptr_t type, intptr_t a0, intptr_t a1, intptr_t a2) {
  register intptr_t _gpr1 asm (GPR1) = type;
  register intptr_t _gpr2 asm (GPR2) = a0;
  register intptr_t _gpr3 asm (GPR3) = a1;
  register intptr_t _gpr4 asm (GPR4) = a2;
  register intptr_t ret asm (GPRx);
  asm volatile(SYSCALL
               : "=r"(ret)
               : "r"(_gpr1), "r"(_gpr2), "r"(_gpr3), "r"(_gpr4));
  return ret;
}

int _open(const char *path, int flags, mode_t mode) {
  // printf("[_open] 尝试打开: %s, flags=%d, mode=%d\n", path, flags, mode);
  int ret = _syscall_(SYS_open, (intptr_t)path, flags, mode);
  // printf("[_open] 系统调用返回: %d (如果<0表示错误)\n", ret);
  return ret;
}

int _write(int fd, void *buf, size_t count) {
  int ret = _syscall_(SYS_write, fd, (intptr_t)buf, count);

  // 特殊处理 _write 的返回值打印，避免无限递归
  // 只有非标准错误输出时才打印，防止递归
  // if (fd != 2) {
  //   char debug_buf[64];
  //   sprintf(debug_buf, "[_write] 系统调用返回: %d\n", ret);
  //   _syscall_(SYS_write, 2, (intptr_t)debug_buf, strlen(debug_buf));
  // }

  return ret;
}

void *_sbrk(intptr_t increment) {
  // 静态变量用于记录当前program break的位置
  // 首次调用时为NULL，后续调用会保留上次的值
  static char *program_break = NULL;
  char *old_break, *new_break;

  // 第一次调用时初始化program_break为_end的位置
  // _end是由链接器提供的符号，表示程序数据段的结束位置
  if (program_break == NULL) {
    extern char _end;      // 声明外部符号_end
    program_break = &_end; // 将program_break初始化为_end符号的地址

    char buf[32];                // 用于存储地址的字符串表示
    sprintf(buf, "%p\n", &_end); // 将地址转换为字符串
    _write(2, buf, strlen(buf)); // 输出地址字符串

    // 调试建议：在此处添加brk初始值的输出
    char debug_buf[64];
    sprintf(debug_buf, "DEBUG: Initial program_break = %p\n", program_break);
    _write(2, debug_buf, strlen(debug_buf));
  }

  old_break = program_break;             // 保存当前program break位置
  new_break = program_break + increment; // 计算新的program break位置

  // 调试建议：添加内存增量信息
  char debug_buf[128];
  sprintf(debug_buf, "DEBUG: _sbrk called with increment=%d, old=%p,new=%p\n",
          increment, old_break, new_break);
  _write(2, debug_buf, strlen(debug_buf));

  // 通过SYS_brk系统调用请求设置新的program break
  // 这里将指针转换为intptr_t类型作为系统调用参数
  int ret = _syscall_(SYS_brk, (intptr_t)new_break, 0, 0);
  sprintf(debug_buf, "[_sbrk]ret=%d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));
  if (ret == 0) {
    // 系统调用成功，更新记录的program break位置
    program_break = new_break;

    // 调试建议：添加系统调用成功信息

    return (void *)
        old_break; // 返回旧的program break位置，这将作为分配内存的起始地址
  } else {
    // 系统调用失败，返回(void *)-1表示错误
    // 调试建议：添加失败信息
    _write(2, "HYBUG: SYS_brk failed\n", 22);
    return (void *)-1;
  }
}

int _read(int fd, void *buf, size_t count) {
  int ret = _syscall_(SYS_read, fd, (intptr_t)buf, count);

  // char debug_buf[64];
  // sprintf(debug_buf, "[_read] 系统调用返回: %d\n", ret);
  // _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

int _close(int fd) {
  int ret = _syscall_(SYS_close, fd, 0, 0);

  // char debug_buf[64];
  // sprintf(debug_buf, "[_close] 系统调用返回: %d\n", ret);
  // _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

off_t _lseek(int fd, off_t offset, int whence) {
  off_t ret = _syscall_(SYS_lseek, fd, offset, whence);

  // char debug_buf[64];
  // sprintf(debug_buf, "[_lseek] 系统调用返回: %ld\n", (long)ret);
  // _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

int _gettimeofday(struct timeval *tv, struct timezone *tz) {
  int ret = _syscall_(SYS_gettimeofday, (intptr_t)tv, (intptr_t)tz, 0);

  // char debug_buf[64];
  // sprintf(debug_buf, "[_gettimeofday] 系统调用返回: %d\n", ret);
  // _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

int _execve(const char *fname, char * const argv[], char *const envp[]) {
  int ret =
      _syscall_(SYS_execve, (intptr_t)fname, (intptr_t)argv, (intptr_t)envp);

  char debug_buf[64];
  sprintf(debug_buf, "[_execve] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

int _fstat(int fd, struct stat *buf) {
  int ret = _syscall_(SYS_fstat, fd, (intptr_t)buf, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_fstat] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

int _kill(int pid, int sig) {
  int ret = _syscall_(SYS_kill, pid, sig, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_kill] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

pid_t _getpid() {
  pid_t ret = _syscall_(SYS_getpid, 0, 0, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_getpid] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

pid_t _fork() {
  pid_t ret = _syscall_(SYS_fork, 0, 0, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_fork] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

pid_t vfork() {
  return _fork(); // 使用_fork实现vfork
}

int _link(const char *d, const char *n) {
  int ret = _syscall_(SYS_link, (intptr_t)d, (intptr_t)n, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_link] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

int _unlink(const char *n) {
  int ret = _syscall_(SYS_unlink, (intptr_t)n, 0, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_unlink] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

pid_t _wait(int *status) {
  pid_t ret = _syscall_(SYS_wait, (intptr_t)status, 0, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_wait] 系统调用返回: %d\n", ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

clock_t _times(void *buf) {
  clock_t ret = _syscall_(SYS_times, (intptr_t)buf, 0, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_times] 系统调用返回: %ld\n", (long)ret);
  _write(2, debug_buf, strlen(debug_buf));

  return ret;
}

void _exit(int status) {
  intptr_t ret = _syscall_(SYS_exit, status, 0, 0);

  char debug_buf[64];
  sprintf(debug_buf, "[_exit] 系统调用返回: %ld\n", (long)ret);
  _write(2, debug_buf, strlen(debug_buf));

  while (1)
    ;
}