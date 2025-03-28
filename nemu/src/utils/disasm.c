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

#include <dlfcn.h>
#include <capstone/capstone.h>
#include <common.h>

static size_t (*cs_disasm_dl)(csh handle, const uint8_t *code,
    size_t code_size, uint64_t address, size_t count, cs_insn **insn);
static void (*cs_free_dl)(cs_insn *insn, size_t count);

static csh handle;

void init_disasm() {//反汇编初始化函数
  void *dl_handle;
  dl_handle = dlopen("tools/capstone/repo/libcapstone.so.5", RTLD_LAZY);
  assert(dl_handle);

  cs_err (*cs_open_dl)(cs_arch arch, cs_mode mode, csh *handle) = NULL;
  cs_open_dl = dlsym(dl_handle, "cs_open");
  assert(cs_open_dl);

  cs_disasm_dl = dlsym(dl_handle, "cs_disasm");
  assert(cs_disasm_dl);

  cs_free_dl = dlsym(dl_handle, "cs_free");
  assert(cs_free_dl);

  cs_arch arch = MUXDEF(CONFIG_ISA_x86,      CS_ARCH_X86,
                   MUXDEF(CONFIG_ISA_mips32, CS_ARCH_MIPS,
                   MUXDEF(CONFIG_ISA_riscv,  CS_ARCH_RISCV,
                   MUXDEF(CONFIG_ISA_loongarch32r,  CS_ARCH_LOONGARCH, -1))));
  cs_mode mode = MUXDEF(CONFIG_ISA_x86,      CS_MODE_32,
                   MUXDEF(CONFIG_ISA_mips32, CS_MODE_MIPS32,
                   MUXDEF(CONFIG_ISA_riscv,  MUXDEF(CONFIG_ISA64, CS_MODE_RISCV64, CS_MODE_RISCV32) | CS_MODE_RISCVC,
                   MUXDEF(CONFIG_ISA_loongarch32r,  CS_MODE_LOONGARCH32, -1))));
	int ret = cs_open_dl(arch, mode, &handle);
  assert(ret == CS_ERR_OK);

#ifdef CONFIG_ISA_x86
  cs_err (*cs_option_dl)(csh handle, cs_opt_type type, size_t value) = NULL;
  cs_option_dl = dlsym(dl_handle, "cs_option");
  assert(cs_option_dl);

  ret = cs_option_dl(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);
  assert(ret == CS_ERR_OK);
#endif
}

void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
	cs_insn *insn;
	size_t count = cs_disasm_dl(handle, code, nbyte, pc, 0, &insn);
  assert(count == 1);
  int ret = snprintf(str, size, "%s", insn->mnemonic);
  if (insn->op_str[0] != '\0') {
    snprintf(str + ret, size - ret, "\t%s", insn->op_str);
  }
  cs_free_dl(insn, count);
}
/*
调用 Capstone 库的 cs_disasm_dl 函数将二进制指令反汇编为 cs_insn 结构
使用 assert 确保成功反汇编了一条指令
使用 snprintf 将指令的助记符（如 add, mov, jmp 等）写入输出字符串
如果指令有操作数（如寄存器名、立即数等），将其添加到输出字符串中
最后释放 Capstone 分配的资源

char *str:
输出参数
用于存储反汇编后的指令字符串
函数执行完成后，这个缓冲区将包含可读的汇编指令文本

int size:
str缓冲区的大小（字节数）
防止缓冲区溢出的安全参数
限制写入输出字符串的最大字符数

uint64_t pc:
程序计数器的值
当前指令的内存地址
在反汇编输出中作为指令的地址显示
对于相对跳转指令的目标计算很重要

uint8_t *code:
指向机器码（二进制指令）的指针
要被反汇编的原始二进制数据
可以是单条指令或多条指令的二进制表示

int nbyte:
要反汇编的机器码的字节长度
指定从code指针开始读取多少字节作为指令
*/