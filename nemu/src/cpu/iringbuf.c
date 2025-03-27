#include <cpu/iringbuf.h>
#include <isa.h>

static iringbuf_entry_t iringbuf[IRINGBUF_SIZE] = {};
static int iringbuf_idx = 0;  // 当前写入位置
static bool iringbuf_full = false;  // 标记缓冲区是否已满

void iringbuf_record(vaddr_t pc, uint32_t inst, const char *asm_buf) {
  // 记录当前指令
  iringbuf[iringbuf_idx].pc = pc;// 保存程序计数器值(指令地址)
  iringbuf[iringbuf_idx].inst = inst;// 保存指令的二进制表示
  //安全复制反汇编文本:
  strncpy(iringbuf[iringbuf_idx].asm_buf, asm_buf, sizeof(iringbuf[0].asm_buf) - 1);
  iringbuf[iringbuf_idx].asm_buf[sizeof(iringbuf[0].asm_buf) - 1] = '\0';
  
  // 更新索引，实现环形效果
  iringbuf_idx = (iringbuf_idx + 1) % IRINGBUF_SIZE;
  
  // 如果索引回到0，说明缓冲区已经被填满过一次
  if (iringbuf_idx == 0) {
    iringbuf_full = true;
  }
}

void iringbuf_display() {
  printf("====== Instruction Ring Buffer ======\n");
  
  int start, count;
  if (iringbuf_full) {
    // 如果缓冲区已满，从当前索引开始显示所有条目
    start = iringbuf_idx;
    count = IRINGBUF_SIZE;
  } else {
    // 否则只显示从0到当前索引的条目
    start = 0;
    count = iringbuf_idx;
  }
  
  // 打印指令缓冲区
  for (int i = 0; i < count; i++) {
    int idx = (start + i) % IRINGBUF_SIZE;
    
    // 在最后一条指令前添加箭头标记
    const char *prefix = (i == count - 1) ? "--> " : "    ";
    
    printf("%s0x%08x: 0x%08x  %s\n", 
           prefix, 
           iringbuf[idx].pc, 
           iringbuf[idx].inst, 
           iringbuf[idx].asm_buf);
  }
  
  printf("===================================\n");
}