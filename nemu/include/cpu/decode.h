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

#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

typedef struct Decode {
  vaddr_t pc;
  vaddr_t snpc; // static next pc 代表顺序执行时的下一个指令地址 snpc = pc + 4
  vaddr_t dnpc; // dynamic next pc 代表实际执行后的下一个指令地址
  ISADecodeInfo isa;
  IFDEF(CONFIG_ITRACE, char logbuf[128]);
} Decode;

// --- pattern matching mechanism ---
__attribute__((always_inline))
static inline void pattern_decode(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }

#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}
/*
hy:最后得到的key是去除末尾通配符后的值，
mask是去除末尾通配符后的掩码（当此位置对应为通配符时置为0），
shift是末尾通配符的数量
目的是进行下面的比较：((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key)
*/
/*
# `pattern_decode` 函数详细解析

这个函数是 NEMU 中指令模式匹配系统的核心，用于将二进制模式字符串转换为高效匹配所需的数据结构。

## 函数定义

```cpp
__attribute__((always_inline))
static inline void pattern_decode(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
```

- `__attribute__((always_inline))` - 编译器指令，强制内联此函数以提高性能
- 参数：
  - `str` - 二进制模式字符串，如 "??????? ????? ????? ??? ????? 00101 11"
  - `len` - 字符串长度
  - 三个输出参数：`key`, `mask`, `shift`

## 核心实现

```cpp
  uint64_t __key = 0, __mask = 0, __shift = 0;
```
初始化三个本地变量用于构建结果。

```cpp
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }
```

这个宏定义了处理模式字符串中每个字符的逻辑：

1. 检查索引是否越界
2. 获取当前字符
3. 如果不是空格，继续处理
4. 确认字符是合法的（'0', '1', 或 '?'）
5. 更新 `__key`：
   - 左移1位
   - 如果字符是'1'，最低位置1；否则置0
6. 更新 `__mask`：
   - 左移1位
   - 如果字符是'?'（通配符），最低位置0；否则置1（需要精确匹配）
7. 更新 `__shift`：
   - 如果字符是'?'，增加1（计数末尾连续的'?'的数量）
   - 否则重置为0（非'?'字符会重置计数）

## 宏展开优化

```cpp
#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
```

这是一种展开优化技术（Duff's device 的变体），通过递归宏展开，使编译器一次处理多个字符，减少循环开销。`macro64(0)` 会处理模式字符串中的前64个字符。

## 完成处理

```cpp
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}
```

1. 如果模式字符串超过64个字符，触发错误
2. 处理完毕后，将结果写入输出参数：
   - `*key` - 将 `__key` 右移 `__shift` 位，去除末尾通配符的影响
   - `*mask` - 同样右移处理 
   - `*shift` - 保存末尾连续通配符的数量

## 工作原理举例

假设模式为 "10??"：

1. 处理 '1'：
   - `__key = 0 << 1 | 1 = 1`
   - `__mask = 0 << 1 | 1 = 1`
   - `__shift = 0`

2. 处理 '0'：
   - `__key = 1 << 1 | 0 = 2`
   - `__mask = 1 << 1 | 1 = 3`
   - `__shift = 0`

3. 处理 '?'：
   - `__key = 2 << 1 | 0 = 4`
   - `__mask = 3 << 1 | 0 = 6`
   - `__shift = 1`

4. 处理 '?'：
   - `__key = 4 << 1 | 0 = 8`
   - `__mask = 6 << 1 | 0 = 12`
   - `__shift = 2`

5. 最终结果：
   - `key = 8 >> 2 = 2`（对应二进制 "10"）
   - `mask = 12 >> 2 = 3`（对应二进制 "11"）
   - `shift = 2`（末尾两个通配符）

随后，当匹配指令时，会将指令右移 `shift` 位，然后与 `mask` 进行按位与，结果与 `key` 比较，实现高效匹配。

这种设计既灵活（支持通配符匹配）又高效（避免了运行时逐位比较），是NEMU指令解码系统的关键部分。
*/

__attribute__((always_inline))
static inline void pattern_decode_hex(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); \
      __shift = (c == '?' ? __shift + 4 : 0); \
    } \
  }
  /*
  检查字符是否有效（0-9, a-f, 或 ?）
  更新 __key：左移 4 位，然后将十六进制字符转为数值（问号为 0）
  更新 __mask：左移 4 位，然后对问号设为 0，其他设为 0xf（表示完全匹配）
  更新 __shift：如果是问号则增加 4（因为每个十六进制字符是 4 位），否则为 0
  */
  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}


// --- 用于解码的模式匹配包装器 ---
#define INSTPAT(pattern, ...) do { \
  uint64_t key, mask, shift; \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) { \
    INSTPAT_MATCH(s, ##__VA_ARGS__); \
    goto *(__instpat_end); \
  } \
} while (0)
/*
pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift);
调用 pattern_decode 函数解析二进制模式字符串（如 "??????? ????? ????? ??? ????? 00101 11"）
将模式转换为三个值：key（匹配值）、mask（匹配位掩码）、shift（位移量）

if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key)
获取当前指令 INSTPAT_INST(s)
将指令右移 shift 位，与 mask 按位与操作，然后与 key 比较
如果结果相等，则表明指令匹配了当前模式

INSTPAT_MATCH(s, ##__VA_ARGS__);
如果模式匹配成功，调用 INSTPAT_MATCH 宏
这个宏会解码指令操作数并执行实际的指令操作（在可变参数中定义）
如：INSTPAT_MATCH(s, auipc, U, R(rd) = s->pc + imm);

goto *(__instpat_end);
使用计算的跳转（computed goto）直接跳到指令模式匹配表的结尾
这是一种优化手段，避免继续进行不必要的模式匹配
*/

#define INSTPAT_START(name) { const void * __instpat_end = &&concat(__instpat_end_, name);
#define INSTPAT_END(name)   concat(__instpat_end_, name): ; }

#endif
