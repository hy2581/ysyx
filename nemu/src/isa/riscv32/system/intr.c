/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */

  // 保存当前PC到mepc
  Log("Saving EPC: 0x%08x", epc);  // 使用%x以十六进制打印地址
  Log("Saving cause: %u", NO);     // 使用%u打印无符号数

  cpu.csr.mepc = epc;

  // 保存异常原因到mcause
  cpu.csr.mcause = NO;

  // RISC-V中断向量基地址存储在mtvec中
  // 不考虑向量模式，直接使用mtvec作为入口地址
  word_t vector = cpu.csr.mtvec;

  return vector;
}
word_t isa_query_intr() {
  return INTR_EMPTY;
}
