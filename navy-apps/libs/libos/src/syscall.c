#include "syscall.h"
#include <assert.h>
#include <stdio.h>
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
  asm volatile (SYSCALL : "=r" (ret) : "r"(_gpr1), "r"(_gpr2), "r"(_gpr3), "r"(_gpr4));
  return ret;
}

void _exit(int status) {
  _syscall_(SYS_exit, status, 0, 0);
  while (1);
}

int _open(const char *path, int flags, mode_t mode) {
  return _syscall_(SYS_open, (intptr_t)path, flags, mode);
}

int _write(int fd, void *buf, size_t count) {
  return _syscall_(SYS_write, fd, (intptr_t)buf, count);
}

void *_sbrk(intptr_t increment) {
  // 静态变量用于记录当前program break的位置
  static char *program_break = NULL;
  char *old_break, *new_break;
  
  // 第一次调用时初始化program_break为_end的位置
  if (program_break == NULL) {
    extern char _end; // 使用外部符号_end（在链接时由ld提供）
    program_break = &_end;
  }

  old_break = program_break; // 保存旧的program break位置
  new_break = program_break + increment; // 计算新的program break位置
  
  // 通过SYS_brk系统调用请求设置新的program break
  int ret = _syscall_(SYS_brk, (intptr_t)new_break, 0, 0);
  
  if (ret == 0) {
    // 系统调用成功，更新记录的program break位置
    program_break = new_break;
    return (void *)old_break; // 返回旧的program break位置
  } else {
    // 系统调用失败
    return (void *)-1;
  }
}

int _read(int fd, void *buf, size_t count) {
  return _syscall_(SYS_read, fd, (intptr_t)buf, count);
}

int _close(int fd) { return _syscall_(SYS_close, fd, 0, 0); }

off_t _lseek(int fd, off_t offset, int whence) {
  return _syscall_(SYS_lseek, fd, offset, whence);
}

int _gettimeofday(struct timeval *tv, struct timezone *tz) {
  return _syscall_(SYS_gettimeofday, (intptr_t)tv, (intptr_t)tz, 0);
}

int _execve(const char *fname, char * const argv[], char *const envp[]) {
  return _syscall_(SYS_execve, (intptr_t)fname, (intptr_t)argv, (intptr_t)envp);
}

// Syscalls below are not used in Nanos-lite.
// But to pass linking, they are defined as dummy functions.

int _fstat(int fd, struct stat *buf) {
  return _syscall_(SYS_fstat, fd, (intptr_t)buf, 0);
}

int _stat(const char *fname, struct stat *buf) {
  printf("未实现");
  assert(0);
  return -1;
}

int _kill(int pid, int sig) { return _syscall_(SYS_kill, pid, sig, 0); }

pid_t _getpid() { return _syscall_(SYS_getpid, 0, 0, 0); }

pid_t _fork() { return _syscall_(SYS_fork, 0, 0, 0); }

pid_t vfork() {
  return _fork(); // 使用_fork实现vfork
}

int _link(const char *d, const char *n) {
  return _syscall_(SYS_link, (intptr_t)d, (intptr_t)n, 0);
}

int _unlink(const char *n) { return _syscall_(SYS_unlink, (intptr_t)n, 0, 0); }

pid_t _wait(int *status) { return _syscall_(SYS_wait, (intptr_t)status, 0, 0); }

clock_t _times(void *buf) { return _syscall_(SYS_times, (intptr_t)buf, 0, 0); }

int pipe(int pipefd[2]) {
  printf("未实现");
  assert(0);
  return -1;
}

int dup(int oldfd) {
  printf("未实现");
  assert(0);
  return -1;
}

int dup2(int oldfd, int newfd) {
  printf("未实现");
  return -1;
}

unsigned int sleep(unsigned int seconds) {
  struct timeval tv_start, tv_end;
  _gettimeofday(&tv_start, NULL);

  // 简单实现，使用循环等待
  while (1) {
    _gettimeofday(&tv_end, NULL);
    if ((tv_end.tv_sec - tv_start.tv_sec) >= seconds) {
      break;
    }
  }

  return 0;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  printf("未实现");
  assert(0);
  return -1;
}

int symlink(const char *target, const char *linkpath) {
  printf("未实现");
  assert(0);
  return -1;
}

int ioctl(int fd, unsigned long request, ...) {
  printf("未实现");
  return -1;
}