#ifndef __FTRACE_H__
#define __FTRACE_H__

#include <common.h>

typedef struct {
  char *name;       // 函数名
  uint32_t addr;    // 函数起始地址
  uint32_t size;    // 函数大小
} FuncInfo;

typedef struct {
  bool enabled;           // ftrace是否启用
  int func_count;         // 函数数量
  int call_depth;         // 当前调用深度
  FuncInfo *functions;    // 函数信息数组
} FtraceState;

void init_ftrace(const char *elf_file);
void ftrace_call(uint32_t pc, uint32_t target);
void ftrace_ret(uint32_t pc, uint32_t target);
FuncInfo* find_function(uint32_t addr);
void cleanup_ftrace();

extern FtraceState ftrace_state;

#endif