#include <NDL.h>
#include <stdio.h>

// 简单的延迟函数
void delay_loop(int n) {
  volatile int i;
  for (i = 0; i < n; i++)
    ;
}

int main() {
  // 初始化NDL
  NDL_Init(0);

  uint32_t prev_ticks = NDL_GetTicks();
  printf("Timer test begins. Press Ctrl+C to exit.\n");

  while (1) {
    // 获取当前时间（毫秒）
    uint32_t now_ticks = NDL_GetTicks();

    // 计算时间差（毫秒）
    uint32_t elapsed_ms = now_ticks - prev_ticks;

    // 每500毫秒（0.5秒）输出一次信息
    if (elapsed_ms >= 500) {
      printf("0.5s passed! Current ticks: %u ms\n", now_ticks);
      prev_ticks = now_ticks; // 更新上次时间
    }

    // 使用简单延迟
    delay_loop(10000);
  }

  // 清理NDL（在这个例子中不会执行到，但为了完整性添加）
  NDL_Quit();
  return 0;
}