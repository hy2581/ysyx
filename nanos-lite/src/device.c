#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

// 串口写入函数
size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; ++i)
    putch(*((char *)buf + i));
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T t = io_read(AM_INPUT_KEYBRD);
  return snprintf((char *)buf, len, "%s %s\n", t.keydown ? "kd" : "ku",
                  keyname[t.keycode]);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  // 更简洁的实现，直接返回格式化字符串长度
  AM_GPU_CONFIG_T t = io_read(AM_GPU_CONFIG);
  return snprintf((char *)buf, len, "WIDTH:%d\nHEIGHT:%d\n", t.width, t.height);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  // 直接使用传入的offset参数，不查找文件表
  printf("fb_write: 偏移量 %d 字节, 长度 %d 字节\n", offset, len);

  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;

  // 将字节偏移量转换为像素单位
  offset /= 4;
  size_t pixels_len = len / 4;

  int y = offset / width;
  int x = offset % width;

  printf("fb_write: 转换为坐标 (%d, %d), 长度 %d 像素\n", x, y, pixels_len);

  // 调用AM绘图接口
  io_write(AM_GPU_FBDRAW, x, y, (void *)buf, pixels_len, 1, true);

  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
