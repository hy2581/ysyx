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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_R, TYPE_B, TYPE_J,  // 添加R型、B型和J型指令格式
  TYPE_N, // none
};


#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = SEXT(BITS(i, 31, 31), 1) << 12 | BITS(i, 7, 7) << 11 | \
  BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1; } while(0)
#define immJ() do { *imm = SEXT(BITS(i, 31, 31), 1) << 20 | BITS(i, 19, 12) << 12 | \
  BITS(i, 20, 20) << 11 | BITS(i, 30, 21) << 1; } while(0)
//hy:可能需要在这里加东西
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);  // 从指令中提取rs1寄存器编号(位15-19)取值范围：0-31，对应 RISC-V 的 32 个通用寄存器
  int rs2 = BITS(i, 24, 20);  // 从指令中提取rs2寄存器编号(位20-24)
  *rd     = BITS(i, 11, 7);   // 从指令中提取目标寄存器编号(位7-11)
  switch (type) {
    case TYPE_I: src1R();          immI(); break;  // I型指令：读取rs1, 提取I型立即数
    case TYPE_U:                   immU(); break;  // U型指令：仅提取U型立即数
    case TYPE_S: src1R(); src2R(); immS(); break;  // S型指令：读取rs1和rs2, 提取S型立即数
    case TYPE_R: src1R(); src2R();         break;  // R型指令：读取rs1和rs2
    case TYPE_B: src1R(); src2R(); immB(); break;  // B型指令：读取rs1和rs2，提取B型立即数
    case TYPE_J:                   immJ(); break;  // J型指令：提取J型立即数
    case TYPE_N: break;                            // N型：不需要操作数(如ebreak)
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;//默认情况下下一pc是静态下一pc

#define INSTPAT_INST(s) ((s)->isa.inst)//获取当前指令
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
//解码指令，获取操作数，然后执行指令体
/*
s - 指令解码结构体 (Decode *类型)，包含当前正在执行的指令的所有信息
name - 指令名称，如auipc、lbu等（仅用于标识）
type - 指令格式类型，如 I(I型指令)、U(U型指令)、S(S型指令)、N(无类型)
execute body  - 可变参数，表示指令的执行代码

rd - 目标寄存器号
src1 - 第一个源操作数（通常是寄存器值）
src2 - 第二个源操作数（通常是寄存器值）
imm - 立即数

*/

  INSTPAT_START();//目的是生成一个标签，用于跳转
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  
  // 添加基本算术运算指令
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  
  // 添加逻辑运算指令
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  
  // 添加移位指令
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << (src2 & 0x1f));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> (src2 & 0x1f));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (sword_t)src1 >> (src2 & 0x1f));
  
  // 添加比较指令
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = ((sword_t)src1 < (sword_t)src2) ? 1 : 0);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (src1 < src2) ? 1 : 0);
  
  // 添加分支指令
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc = (src1 == src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s->dnpc = (src1 != src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, s->dnpc = ((sword_t)src1 < (sword_t)src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc = ((sword_t)src1 >= (sword_t)src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc = (src1 < src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc = (src1 >= src2) ? s->pc + imm : s->snpc);
  
  // 添加跳转指令
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->pc + 4; s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->pc + 4; s->dnpc = (src1 + imm) & ~1);
  
  // 添加内存加载指令
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  
  // 添加内存存储指令
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  
  // lui指令
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);

  // 已有的特殊指令
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10)));
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}//hy:可能需要在这里加东西

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  /*
  调用 inst_fetch 函数从当前指令地址 (s->snpc) 处获取一个 4 字节的指令
  指令被读取并存储在 s->isa.inst 中
  &s->snpc 作为引用参数传递，在取指后会被更新为下一条
  */
  return decode_exec(s);
}
