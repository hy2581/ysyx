#include <am.h>
#include <nemu.h>

static uint64_t boot_time = 0;

void __am_timer_init() {
  // 记录启动时间，便于计算相对时间
  uint32_t low = inl(RTC_ADDR);      // 读取低32位
  uint32_t high = inl(RTC_ADDR + 4); // 读取高32位
  boot_time = ((uint64_t)high << 32) | low;
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 读取当前RTC值
  uint32_t low = inl(RTC_ADDR);      // 读取低32位
  uint32_t high = inl(RTC_ADDR + 4); // 读取高32位
  uint64_t current_time = ((uint64_t)high << 32) | low;

  // 计算并返回自启动以来经过的微秒数
  uptime->us = current_time;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  // 根据说明，我们不需要实现此功能，直接返回1900年
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
