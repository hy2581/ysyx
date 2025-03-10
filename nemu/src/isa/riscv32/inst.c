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
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);  // 从指令中提取rs1寄存器编号(位15-19)取值范围：0-31，对应 RISC-V 的 32 个通用寄存器
  int rs2 = BITS(i, 24, 20);  // 从指令中提取rs2寄存器编号(位20-24)
  *rd     = BITS(i, 11, 7);   // 从指令中提取目标寄存器编号(位7-11)
  switch (type) {
    case TYPE_I: src1R();          immI(); break;  // I型指令：读取rs1, 提取I型立即数
    case TYPE_U:                   immU(); break;  // U型指令：仅提取U型立即数
    case TYPE_S: src1R(); src2R(); immS(); break;  // S型指令：读取rs1和rs2, 提取S型立即数
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

调用 decode_operand 函数解析指令中的操作数
使用 concat(TYPE_, type) 连接 TYPE_ 和参数 type，形成完整的类型名
例如，对于I型指令，会变成 TYPE_I

decode_operand 函数根据指令类型解码出不同的操作数：

对于 I 型指令：获取 rs1 寄存器值和 I 型立即数
对于 U 型指令：仅获取 U 型立即数
对于 S 型指令：获取两个寄存器值 rs1、rs2 和 S 型立即数
对于 N 型指令：不提取操作数

最后执行R(rd) = s->pc + imm;
*/

  INSTPAT_START();//目的是生成一个标签，用于跳转
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);//将PC值与U型立即数相加，结果写入目标寄存器
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));//从内存加载一个字节（无符号），存入寄存器
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));//将寄存器中的一个字节存入内存

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // 触发环境断点
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));//处理无法识别的指令
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  /*
  调用 inst_fetch 函数从当前指令地址 (s->snpc) 处获取一个 4 字节的指令
  指令被读取并存储在 s->isa.inst 中
  &s->snpc 作为引用参数传递，在取指后会被更新为下一条
  */
  return decode_exec(s);
}
