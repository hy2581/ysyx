#include <am.h>
#include <klib.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  // 读取i8042数据寄存器获取键盘码
  uint32_t kbd_code =
      *(volatile uint32_t *)KBD_ADDR; //感觉是inl没有正确读出地址里的数！！！

  // 解析键盘码
  kbd->keydown = (kbd_code & KEYDOWN_MASK ? true : false);
  kbd->keycode = kbd_code & ~KEYDOWN_MASK;

  // 如果没有按键，将keycode设为AM_KEY_NONE
  if (kbd_code == 0) {
    kbd->keycode = AM_KEY_NONE;
  }
  // 读取i8042状态寄存器，清除中断标志
}