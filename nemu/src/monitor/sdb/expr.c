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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include "common.h"

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM,
  // 可以在这里添加更多 token 类型
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  // 忽略空白符
  {" +",    TK_NOTYPE},
  // 运算符
  {"\\+",   '+'},
  {"-",     '-'},
  {"\\*",   '*'},
  {"/",     '/'},
  {"\\(",   '('},
  {"\\)",   ')'},
  // 关系运算符
  {"==",    TK_EQ},
  // 数字(十进制)
  {"[0-9]+", TK_NUM},
};

#define NR_REGEX ARRLEN(rules)
static regex_t re[NR_REGEX] = {};

/* 在程序初始化时编译正则表达式 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

/* 利用正则匹配将表达式拆分成若干 token */
static bool make_token(char *e) {
  int position = 0;
  nr_token = 0;

  while (e[position] != '\0') {
    bool matched = false;
    for (int i = 0; i < NR_REGEX; i ++) {
      regmatch_t pmatch;
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        position += substr_len;
        matched = true;

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            // 空白符，不添加 token
            break;

          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
          case TK_EQ:
            // 运算符或关系运算符
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token++;
            break;

          case TK_NUM: {
            // 数字类型，需要记录其字符串表示
            tokens[nr_token].type = TK_NUM;
            int len = (substr_len < (int)sizeof(tokens[nr_token].str))
                      ? substr_len : (int)sizeof(tokens[nr_token].str) - 1;
            strncpy(tokens[nr_token].str, substr_start, len);
            tokens[nr_token].str[len] = '\0';
            nr_token++;
            break;
          }

          default:
            panic("Unhandled token type: %d", rules[i].token_type);
            break;
        }
        break;
      }
    }
    if (!matched) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/* 递归求值函数示例，可根据需要细化 */
static bool check_parentheses(int p, int q) {
  if (tokens[p].type == '(' && tokens[q].type == ')') {
    int balance = 0;
    for (int i = p; i <= q; i++) {
      if (tokens[i].type == '(') balance++;
      else if (tokens[i].type == ')') balance--;
      if (balance == 0 && i < q) return false;
    }
    return (balance == 0);
  }
  return false;
}

static word_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }
  if (p == q) {
    // 单个 token
    if (tokens[p].type == TK_NUM) {
      return strtoul(tokens[p].str, NULL, 10);
    }
    else {
      *success = false;
      return 0;
    }
  }
  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  // 寻找主运算符(改进方式)
  int op = -1;
  int level_min = 9999;
  int paren_count = 0;

  for (int i = p; i <= q; i++) {
    // 更新括号计数，上遇 '('++，遇 ')'--
    if (tokens[i].type == '(') {
      paren_count++;
    } else if (tokens[i].type == ')') {
      paren_count--;
    }

    // 只在外层（paren_count == 0）考虑运算符，跳过嵌套括号内的运算符
    if (paren_count == 0) {
      int level = 0;
      if (tokens[i].type == '+' || tokens[i].type == '-') level = 1;
      if (tokens[i].type == '*' || tokens[i].type == '/') level = 2;

      // 记录“最低优先级”且最右侧的运算符下标
      if (level <= level_min && level > 0) {
        level_min = level;
        op = i;
      }
    }
  }

  if (op == -1) {
    *success = false;
    return 0;
  }

  word_t val1 = eval(p, op - 1, success);
  if (!*success) return 0;
  word_t val2 = eval(op + 1, q, success);
  if (!*success) return 0;

  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/':
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      return val1 / val2;
    default: 
      *success = false;
      return 0;
  }
}

/* 外部使用的表达式求值接口 */
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  return eval(0, nr_token - 1, success);
}
