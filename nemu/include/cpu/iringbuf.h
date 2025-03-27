#ifndef __CPU_IRINGBUF_H__
#define __CPU_IRINGBUF_H__

#include <common.h>

#define IRINGBUF_SIZE 16  // 环形缓冲区大小，可根据需求调整

typedef struct {
  vaddr_t pc;           // 指令地址
  uint32_t inst;        // 指令内容
  char asm_buf[128];    // 反汇编后的指令字符串
} iringbuf_entry_t;

// 在环形缓冲区中记录一条指令
void iringbuf_record(vaddr_t pc, uint32_t inst, const char *asm_buf);

// 打印环形缓冲区内容
void iringbuf_display();

#endif