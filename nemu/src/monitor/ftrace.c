// 包含ELF文件格式定义的头文件，提供了Elf32_Ehdr、Elf32_Shdr等结构体定义
#include <elf.h>
// 包含标准输入输出函数(如printf)的头文件  
#include <stdio.h>
// 包含内存分配函数(如malloc、free)的头文件
#include <stdlib.h>
// 包含字符串处理函数(如strcmp、strdup)的头文件
#include <string.h>
// 包含自定义的ftrace功能相关的头文件定义
#include <monitor/ftrace.h>

// 初始化全局ftrace状态变量，{0}表示所有成员初始化为0或NULL
FtraceState ftrace_state = {0};

// 初始化函数追踪功能的函数，参数是ELF文件的路径
void init_ftrace(const char *elf_file) {
  // 检查ELF文件路径是否为空
  if (elf_file == NULL) {
    printf("No ELF file provided for ftrace\n");
    return;  // 如果路径为空，打印错误信息并返回
  }
  
  // 以二进制读模式打开ELF文件
  FILE *fp = fopen(elf_file, "rb");
  if (fp == NULL) {
    printf("Cannot open ELF file '%s'\n", elf_file);
    return;  // 如果文件打开失败，打印错误信息并返回
  }
  
  // 声明一个ELF头结构体变量
  Elf32_Ehdr elf_header;
  // 从文件读取ELF头信息到elf_header结构体
  fread(&elf_header, sizeof(Elf32_Ehdr), 1, fp);
  
  // 验证文件是否为有效的ELF文件，通过检查魔数(Magic Number)
  // ELFMAG是标准ELF魔数，SELFMAG是魔数的长度
  if (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
    printf("Invalid ELF file\n");
    fclose(fp);  // 关闭文件
    return;      // 如果魔数不匹配，文件不是有效的ELF文件，返回
  }
  
  // 为节头表分配内存，大小为节头项大小乘以节头表中的项数
  Elf32_Shdr *sh_table = malloc(sizeof(Elf32_Shdr) * elf_header.e_shnum);
  // 将文件指针移动到节头表在文件中的位置
  fseek(fp, elf_header.e_shoff, SEEK_SET);
  // 读取整个节头表到分配的内存中
  fread(sh_table, sizeof(Elf32_Shdr), elf_header.e_shnum, fp);
  
  // 获取节名字符串表的节头
  // e_shstrndx是节头字符串表在节头表中的索引
  Elf32_Shdr *sh_strtab = &sh_table[elf_header.e_shstrndx];
  // 为节名字符串表内容分配内存
  char *sh_strtab_p = malloc(sh_strtab->sh_size);
  // 将文件指针移动到节名字符串表在文件中的位置
  fseek(fp, sh_strtab->sh_offset, SEEK_SET);
  // 读取节名字符串表内容到分配的内存中
  fread(sh_strtab_p, sh_strtab->sh_size, 1, fp);
  
  // 声明用于存储符号表和符号名称字符串表节头的指针
  Elf32_Shdr *symtab = NULL, *strtab = NULL;
  // 遍历所有节，查找符号表(.symtab)和字符串表(.strtab)节
  for (int i = 0; i < elf_header.e_shnum; i++) {
    // 获取节名，sh_name是节名在字符串表中的偏移
    char *name = sh_strtab_p + sh_table[i].sh_name;
    // 比较节名，寻找符号表节
    if (strcmp(name, ".symtab") == 0) {
      symtab = &sh_table[i];  // 找到符号表节，保存节头指针
    } 
    // 比较节名，寻找字符串表节
    else if (strcmp(name, ".strtab") == 0) {
      strtab = &sh_table[i];  // 找到字符串表节，保存节头指针
    }
  }
  
  // 检查是否找到了符号表和字符串表
  if (!symtab || !strtab) {
    printf("Symbol table or string table not found\n");
    // 释放已分配的内存，避免内存泄漏
    free(sh_strtab_p);
    free(sh_table);
    fclose(fp);  // 关闭文件
    return;      // 如果找不到必要的表，返回
  }
  
  // 计算符号表中的符号数量，为符号表的大小除以单个符号的大小
  int sym_count = symtab->sh_size / sizeof(Elf32_Sym);
  // 为符号表内容分配内存
  Elf32_Sym *syms = malloc(symtab->sh_size);
  // 将文件指针移动到符号表在文件中的位置
  fseek(fp, symtab->sh_offset, SEEK_SET);
  // 读取符号表内容到分配的内存中
  fread(syms, symtab->sh_size, 1, fp);
  
  // 为符号名称字符串表内容分配内存
  char *str_table = malloc(strtab->sh_size);
  // 将文件指针移动到字符串表在文件中的位置
  fseek(fp, strtab->sh_offset, SEEK_SET);
  // 读取字符串表内容到分配的内存中
  fread(str_table, strtab->sh_size, 1, fp);
  
  // 统计函数类型符号的数量
  int func_count = 0;
  for (int i = 0; i < sym_count; i++) {
    // ELF32_ST_TYPE宏从st_info字段提取符号类型
    // STT_FUNC是函数类型的符号
    if (ELF32_ST_TYPE(syms[i].st_info) == STT_FUNC) {
      func_count++;  // 如果是函数符号，计数加一
    }
  }
  
  // 为函数信息数组分配内存
  ftrace_state.functions = malloc(sizeof(FuncInfo) * func_count);
  // 保存找到的函数数量
  ftrace_state.func_count = func_count;
  
  // 从符号表中提取函数信息
  int idx = 0;  // 函数信息数组的索引
  for (int i = 0; i < sym_count; i++) {
    // 再次检查是否是函数符号
    if (ELF32_ST_TYPE(syms[i].st_info) == STT_FUNC) {
      // 复制函数名，st_name是符号名在字符串表中的偏移
      ftrace_state.functions[idx].name = strdup(str_table + syms[i].st_name);
      // 保存函数地址
      ftrace_state.functions[idx].addr = syms[i].st_value;
      // 保存函数大小
      ftrace_state.functions[idx].size = syms[i].st_size;
      idx++;  // 移动到下一个函数信息位置
    }
  }
  
  // 启用ftrace功能
  ftrace_state.enabled = true;
  // 初始化调用深度为0
  ftrace_state.call_depth = 0;
  
  // 释放临时分配的内存，避免内存泄漏
  free(str_table);    // 释放符号名称字符串表内存
  free(syms);         // 释放符号表内存
  free(sh_strtab_p);  // 释放节名字符串表内存
  free(sh_table);     // 释放节头表内存
  fclose(fp);         // 关闭ELF文件
  
  // 打印初始化成功信息，显示找到的函数数量
  printf("Ftrace initialized with %d functions\n", func_count);
}

// 根据地址查找对应的函数
FuncInfo* find_function(uint32_t addr) {
  // 如果ftrace未启用，直接返回NULL
  if (!ftrace_state.enabled) return NULL;
  
  // 遍历所有函数，查找包含给定地址的函数
  for (int i = 0; i < ftrace_state.func_count; i++) {
    FuncInfo *func = &ftrace_state.functions[i];
    // 检查地址是否在函数的地址范围内
    // 函数范围是从起始地址到起始地址加上函数大小
    if (addr >= func->addr && addr < func->addr + func->size) {
      return func;  // 找到包含该地址的函数，返回函数信息
    }
  }
  return NULL;  // 没有找到包含该地址的函数，返回NULL
}

// 处理函数调用
void ftrace_call(uint32_t pc, uint32_t target) {
  // 如果ftrace未启用，直接返回
  if (!ftrace_state.enabled) return;
  
  // 查找目标地址对应的函数
  FuncInfo *func = find_function(target);
  if (func) {
    // 找到函数，打印调用信息
    // %*s 创建缩进，缩进量等于call_depth * 2个空格
    // 0x%08x 以16进制格式打印PC地址，保证8位宽度
    // %s 打印函数名
    printf("[call] %*s0x%08x: %s\n", 
           ftrace_state.call_depth * 2, "", 
           pc, func->name);
    // 增加调用深度，用于后续输出的缩进
    ftrace_state.call_depth++;
  }
}

// 处理函数返回
void ftrace_ret(uint32_t pc, uint32_t target) {
  // 如果ftrace未启用，直接返回
  if (!ftrace_state.enabled) return;
  
  // 检查调用深度是否大于0，防止深度变为负数
  if (ftrace_state.call_depth > 0) {
    // 减少调用深度，因为正在从一个函数返回
    ftrace_state.call_depth--;
    // 查找当前PC地址对应的函数
    FuncInfo *func = find_function(pc);
    // 打印返回信息
    // 格式类似调用信息，但使用[ret]标识
    // 如果找不到函数名，使用"???"表示
    printf("[ret]  %*s0x%08x: %s\n", 
           ftrace_state.call_depth * 2, "", 
           pc, func ? func->name : "???");
  }
}

// 清理ftrace功能，释放分配的内存
void cleanup_ftrace() {
  // 如果ftrace未启用，直接返回
  if (!ftrace_state.enabled) return;
  
  // 释放所有分配的内存
  for (int i = 0; i < ftrace_state.func_count; i++) {
    // 释放每个函数名的内存，这些是通过strdup分配的
    free(ftrace_state.functions[i].name);
  }
  // 释放函数信息数组的内存
  free(ftrace_state.functions);
  // 标记ftrace为禁用状态
  ftrace_state.enabled = false;
}