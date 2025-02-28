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

// 包含指令集相关的头文件
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
// 包含正则表达式处理库，用于解析表达式
#include <regex.h>
// 包含通用功能和定义的头文件
#include "common.h"
#include <memory/paddr.h>

//得到寄存器值函数，主要用于封装isa_reg_str2val
static word_t get_register_value(const char *reg_name) {
  bool success = true;
  word_t val = isa_reg_str2val(reg_name, &success);
  if (!success) {
    printf("Invalid register name: %s\n", reg_name);
  }
  return val;
}

// 定义token类型的枚举
enum {
  TK_NOTYPE = 256, // 空白字符的类型，从256开始避免与ASCII值冲突
  TK_EQ,           // 等于号(==)的类型
  TK_NUM,          // 数字的类型
  // 可以在这里添加更多 token 类型
  TK_HEX,       // 十六进制数字
  TK_REG,       // 寄存器名称
  TK_NEQ,       // 不等于运算符
  TK_AND,       // 逻辑与运算符
  TK_DEREF,     // 指针解引用(一元*)
};

// 定义正则表达式规则结构体
static struct rule {
  const char *regex;   // 正则表达式字符串
  int token_type;      // 对应的token类型
} rules[] = {
  // 忽略空白符（一个或多个空格）
  {" +",    TK_NOTYPE},
  // 运算符的正则表达式规则
  {"\\+",   '+'},      // 加号 (+) 需要转义
  {"-",     '-'},      // 减号 (-)
  {"\\*",   '*'},      // 乘号 (*) 需要转义
  {"/",     '/'},      // 除号 (/)
  {"\\(",   '('},      // 左括号，需要转义
  {"\\)",   ')'},      // 右括号，需要转义
  // 关系运算符
  {"==",    TK_EQ},    // 等于号(==)
  // 数字(十进制)，一个或多个数字
  {"[0-9]+", TK_NUM},
  {"0[xX][0-9a-fA-F]+", TK_HEX},  // 先处理十六进制
  {"[0-9]+", TK_NUM},             // 再处理十进制
  {"!=", TK_NEQ},                 // 不等于运算符
  {"&&", TK_AND},                 // 逻辑与运算符
};

// 定义规则数组的长度
#define NR_REGEX ARRLEN(rules)
// 编译后的正则表达式数组
static regex_t re[NR_REGEX] = {};

/* 在程序初始化时编译正则表达式 */
void init_regex() {
  int i;
  char error_msg[128];  // 存储错误信息的缓冲区
  int ret;              // 存储返回值

  // 遍历所有规则，编译正则表达式
  for (i = 0; i < NR_REGEX; i ++) {
    // 编译正则表达式，REG_EXTENDED表示使用扩展正则表达式
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {  // 如果编译失败
      // 获取错误信息
      regerror(ret, &re[i], error_msg, 128);
      // 输出错误信息并终止程序
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

// 定义token结构体，用于存储解析后的单个token
typedef struct token {
  int type;      // token类型
  char str[32];  // token字符串内容，主要用于存储数字值
} Token;

// 存储所有解析出的tokens，最多32个
static Token tokens[32] __attribute__((used)) = {};
// tokens数组中有效token的数量
static int nr_token __attribute__((used))  = 0;

/* 利用正则匹配将表达式拆分成若干 token */
static bool make_token(char *e) {
  int position = 0;     // 当前处理位置
  nr_token = 0;         // 重置token计数

  // 循环直到字符串结束
  while (e[position] != '\0') {
    bool matched = false;  // 标记是否匹配成功
    // 尝试用每一个正则表达式去匹配
    for (int i = 0; i < NR_REGEX; i ++) {
      regmatch_t pmatch;  // 存储匹配结果
      // 从当前位置开始匹配，pmatch存储匹配的起始位置和结束位置
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;  // 子字符串开始位置
        int substr_len = pmatch.rm_eo;      // 子字符串长度
        position += substr_len;             // 更新处理位置
        matched = true;                     // 标记为匹配成功

        // 根据匹配到的token类型进行处理
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            // 空白符，不添加 token，直接跳过
            break;

          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
          case TK_EQ:
            // 运算符或关系运算符，记录类型，不需要存储字符内容
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';  // 置空字符串
            nr_token++;  // token计数增加
            break;

          case TK_NUM: {
            // 数字类型，需要记录其字符串表示
            tokens[nr_token].type = TK_NUM;
            // 计算要复制的长度，避免缓冲区溢出
            int len = (substr_len < (int)sizeof(tokens[nr_token].str))
                      ? substr_len : (int)sizeof(tokens[nr_token].str) - 1;
            // 复制数字字符串
            strncpy(tokens[nr_token].str, substr_start, len);
            tokens[nr_token].str[len] = '\0';  // 添加字符串结束符
            nr_token++;  // token计数增加
            break;
          }
          case TK_HEX:
          case TK_REG:
            tokens[nr_token].type = rules[i].token_type;
            int len = (substr_len < (int)sizeof(tokens[nr_token].str))
                      ? substr_len : (int)sizeof(tokens[nr_token].str) - 1;
            strncpy(tokens[nr_token].str, substr_start, len);
            tokens[nr_token].str[len] = '\0';
            nr_token++;
            break;
            
          case TK_NEQ:
          case TK_AND:
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token++;
            break;

          default:
            // 未处理的token类型，报错并退出
            panic("Unhandled token type: %d", rules[i].token_type);
            break;
        }
        break;  // 匹配成功后退出内层循环
      }
    }
    //识别指针解引用运算符
    for (int i = 0; i < nr_token; i++) {
      if (tokens[i].type == '*' && (i == 0 || (
          tokens[i-1].type != TK_NUM && 
          tokens[i-1].type != TK_HEX && 
          tokens[i-1].type != TK_REG && 
          tokens[i-1].type != ')'))) {
        // 将一元*标记为解引用
        tokens[i].type = TK_DEREF;
      }
    }
    if (!matched) {  // 如果没有任何规则匹配成功
      // 输出错误信息，显示无法匹配的位置
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;  // 返回失败
    }
  }

  return true;  // 所有字符都成功解析，返回成功
}

/* 检查从p到q的token是否被一对*完整*的括号包围 */
static bool check_parentheses(int p, int q) {
  // 检查是否以左括号开始，右括号结束
  if (tokens[p].type == '(' && tokens[q].type == ')') {
    int balance = 0;  // 用于跟踪括号平衡
    // 遍历从p到q的所有token
    for (int i = p; i <= q; i++) {
      if (tokens[i].type == '(') balance++;      // 遇到左括号，平衡值增加
      else if (tokens[i].type == ')') balance--; // 遇到右括号，平衡值减少
      // 如果平衡值为0且未到达最后一个token，说明这不是一个完整的括号对
      if (balance == 0 && i < q) return false;
    }
    // 如果最终平衡值为0，说明括号匹配
    return (balance == 0);
  }
  return false;  // 不是以左括号开始和右括号结束
}

/* 递归求值函数，计算从p到q的表达式值 */
static word_t eval(int p, int q, bool *success) {
  // 如果p > q，表示表达式无效
  if (p > q) {
    *success = false;  // 设置失败标志
    return 0;
  }
  
  // 如果只有一个token，直接返回其值
  if (p == q) {
    if (tokens[p].type == TK_NUM) {
      return strtoul(tokens[p].str, NULL, 10);
    }
    else if (tokens[p].type == TK_HEX) {
      // 处理十六进制，跳过"0x"前缀
      return strtoul(tokens[p].str, NULL, 16);
    }
    else if (tokens[p].type == TK_REG) {
      // 获取寄存器值的函数(需要实现)
      return get_register_value(tokens[p].str + 1);
    }
    else {
      *success = false;
      return 0;
    }
  }
  
  // 处理解引用运算符
  if (tokens[p].type == TK_DEREF && p < q) {
    // 计算解引用的操作数
    word_t addr = eval(p + 1, q, success);
    if (!*success) return 0;
    
    // 读取内存中的值
    return paddr_read(addr, 4);  // 假设读取4字节，根据实际需要调整
  }

  // 如果表达式被一对括号完整包围，去除这对括号，递归求值
  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  // 寻找主运算符(最后被计算的运算符)
  int op = -1;           // 主运算符的位置
  int level_min = 9999;  // 运算符的最低优先级
  int paren_count = 0;   // 括号计数

  // 从左向右遍历所有token
  for (int i = p; i <= q; i++) {
    // 更新括号计数
    if (tokens[i].type == '(') {
      paren_count++;  // 遇到左括号，嵌套层次加一
    } else if (tokens[i].type == ')') {
      paren_count--;  // 遇到右括号，嵌套层次减一
    }

    // 只在最外层(没有括号嵌套)考虑运算符
    if (paren_count == 0) {
      int level = 0;  // 运算符优先级
      // 设置优先级：逻辑与最低，然后是相等/不等，加减，乘除最高
      if (tokens[i].type == TK_AND) level = 1;  // 逻辑与
      if (tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ) level = 2; // 相等/不等
      if (tokens[i].type == '+' || tokens[i].type == '-') level = 3;  // 加减
      if (tokens[i].type == '*' || tokens[i].type == '/') level = 4;  // 乘除

      // 记录"最低优先级"且最右侧的运算符下标
      if (level > 0 && level <= level_min) {
        level_min = level;
        op = i;
      }
    }
  }

  // 如果没有找到合适的运算符
  if (op == -1) {
    *success = false;  // 设置失败标志
    return 0;
  }

  // 递归计算左边表达式的值
  word_t val1 = eval(p, op - 1, success);
  if (!*success) return 0;  // 如果左表达式计算失败，直接返回
  
  // 递归计算右边表达式的值
  word_t val2 = eval(op + 1, q, success);
  if (!*success) return 0;  // 如果右表达式计算失败，直接返回

  // 根据运算符类型计算结果
  switch (tokens[op].type) {
    case '+': return val1 + val2;  // 加法
    case '-': return val1 - val2;  // 减法
    case '*': return val1 * val2;  // 乘法
    case '/':
      {if (val2 == 0) {  // 检查除数是否为0
        *success = false;  // 除数为0，设置失败标志
        return 0;
      }
      return val1 / val2;  // 除法
      }
    case TK_EQ: return val1 == val2;  // 相等比较
    case TK_NEQ: return val1 != val2;  // 不等比较
    case TK_AND: return val1 && val2;  // 逻辑与
    default: 
      *success = false;  // 设置失败标志
      return 0;
  }
}

/* 外部使用的表达式求值接口 */
word_t expr(char *e, bool *success) {
  // 先进行词法分析，将表达式切分为token
  if (!make_token(e)) {
    *success = false;  // 词法分析失败，设置失败标志
    return 0;
  }
  // 调用递归求值函数，计算整个表达式的值
  return eval(0, nr_token - 1, success);
}
