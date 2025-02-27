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

#include "sdb.h"

#define NR_WP 32



static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

// 从空闲链表获取一个监视点
WP* new_wp() {
  if (free_ == NULL) {
    // 没有空闲监视点，程序终止
    assert(0);
    return NULL;
  }
  
  // 从free_链表中取出一个节点
  WP *wp = free_;
  free_ = free_->next;
  
  // 将节点添加到head链表头部
  wp->next = head;
  head = wp;
  
  return wp;
}

// 释放一个监视点，将其归还到空闲链表
void free_wp(WP *wp) {
  if (wp == NULL) return;
  
  // 如果要释放的是头节点
  if (head == wp) {
    head = wp->next;
  } else {
    // 在链表中查找wp的前一个节点
    WP *prev = head;
    while (prev != NULL && prev->next != wp) {
      prev = prev->next;
    }
    
    if (prev == NULL) {
      // wp不在使用中的链表中
      printf("Error: watchpoint not found\n");
      return;
    }
    
    // 从链表中移除wp
    prev->next = wp->next;
  }
  
  // 将wp添加到free_链表的头部
  wp->next = free_;
  free_ = wp;
  
  // 清除表达式（可选）
  wp->expr[0] = '\0';
}

// 根据编号查找监视点   
WP* find_wp(int NO) {
  WP *p = head;
  while (p != NULL) {
    if (p->NO == NO) {
      return p;
    }
    p = p->next;
  }
  return NULL;
}

// 打印所有监视点
void list_watchpoints() {
  if (head == NULL) {
    printf("No watchpoints\n");
    return;
  }
  
  printf("Num\tExpr\t\tValue\n");
  WP *p = head;
  while (p != NULL) {
    printf("%d\t%s\t\t0x%08x\n", p->NO, p->expr, p->old_val);
    p = p->next;
  }
}

// 检查所有监视点，返回是否有监视点被触发
bool check_watchpoints() {
  WP *p = head;
  bool triggered = false;
  
  while (p != NULL) {
    bool success;
    uint32_t new_val = expr(p->expr, &success); // 重新计算表达式的值
    
    if (!success) {
      printf("Error: Failed to evaluate watchpoint expression '%s'\n", p->expr);
      p = p->next;
      continue;
    }
    
    if (new_val != p->old_val) {
      // 监视点触发，打印提示信息
      printf("Watchpoint %d: %s\n", p->NO, p->expr);
      printf("Old value = 0x%08x\n", p->old_val);
      printf("New value = 0x%08x\n", new_val);
      p->old_val = new_val; // 更新旧值
      triggered = true;
    }
    
    p = p->next;
  }
  
  return triggered; // 如果有监视点被触发，返回true
}

