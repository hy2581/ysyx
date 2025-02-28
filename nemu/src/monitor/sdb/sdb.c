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
* MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_n(char *args){
  cpu_exec(1);
  return 0;
}

static int cmd_info(char *args) {
  if (args == NULL) {
    printf("Invalid info command\n");
    return 0;
  }
  
  if (args[0] == 'r') {
    // 显示寄存器信息（已有）
    isa_reg_display();
  } else if (args[0] == 'w') {
    // 显示监视点信息
    list_watchpoints();
  } else {
    printf("Unknown info command: '%s'\n", args);
  }
  
  return 0;
}

static int cmd_x(char *args) {
  // 从命令参数中提取扫描个数 N
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  int n = atoi(arg);
  // 提取表达式EXPR，本简化版本只允许十六进制字面值
  char *expr = strtok(NULL, " ");
  if (expr == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  // 解析表达式为内存起始地址
  
  uint32_t addr = strtoul(expr, NULL, 0);
  // 循环读取内存数据并以十六进制打印
  for (int i = 0; i < n; i++) {
    word_t data = paddr_read(addr + i * 4, 4); // 此处按字为单位，每次读取四个字节数据
    printf("0x%08x: 0x%08x\n", addr + i * 4, data);
  }
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }
  bool success = true;
  word_t result = expr(args, &success); // 调用 expr 函数对表达式求值
  if (!success) {
    printf("Bad expression.\n");
  } else {
    printf("%s = %u (0x%x)\n", args, result, result);
  }
  return 0;
}

// 设置监视点
static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Please specify an expression\n");
    return 0;
  }
  
  bool success = true;
  uint32_t val = expr(args, &success);
  
  if (!success) {
    printf("Failed to evaluate expression '%s'\n", args);
    return 0;
  }
  
  WP *wp = new_wp();
  if (wp == NULL) {
    printf("Failed to create watchpoint\n");
    return 0;
  }
  
  strncpy(wp->expr, args, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0'; // 确保字符串结束
  wp->old_val = val;
  
  printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
  
  return 0;
}

// 删除监视点
static int cmd_d(char *args) {
  if (args == NULL) {
    printf("Please specify the watchpoint number\n");
    return 0;
  }
  
  char *endptr;
  int no = strtol(args, &endptr, 10);
  
  if (*endptr != '\0') {
    printf("Invalid watchpoint number: %s\n", args);
    return 0;
  }
  
  WP *wp = find_wp(no);
  if (wp == NULL) {
    printf("Watchpoint %d not found\n", no);
    return 0;
  }
  
  free_wp(wp);
  printf("Deleted watchpoint %d\n", no);
  
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "n", "Execute the next instruction", cmd_n},
  { "info", "Print the information of registers or watchpoints", cmd_info},
  { "x", "Scan memory. Usage: x N EXPR", cmd_x },
  { "p", "Evaluate expression. Usage: p EXPR", cmd_p },
  { "w", "Set a watchpoint. Usage: w EXPR", cmd_w },
  { "d", "Delete a watchpoint. Usage: d N", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
