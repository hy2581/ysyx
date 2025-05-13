#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  // uint32_t size_info = inl(VGACTL_ADDR);
  // int w = size_info >> 16;
  // int h = size_info & 0xFFFF;
  // uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;

  // for (int i = 0; i < w * h; i++) {
  //   fb[i] = i;
  // }

  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t size_info = inl(VGACTL_ADDR);
  uint32_t width = size_info >> 16;
  uint32_t height = size_info & 0xFFFF;

  *cfg = (AM_GPU_CONFIG_T){.present = true,
                           .has_accel = false,
                           .width = width,
                           .height = height,
                           .vmemsz = width * height * sizeof(uint32_t)};
}

/*这个函数是抽象机器(AM)中实现GPU帧缓冲区绘制的核心功能。它的主要作用是将一块矩形区域的像素数据绘制到屏幕上。
从传入的控制结构体 ctl 中提取绘图的关键信息：
(x, y)：要绘制的矩形区域左上角坐标
(w, h)：矩形区域的宽度和高度
pixels：要绘制的像素数据数组*/
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  uint32_t *pixels = ctl->pixels;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;

  uint32_t size_info = inl(VGACTL_ADDR);
  uint32_t screen_w = size_info >> 16;
  uint32_t screen_h = size_info & 0xFFFF;

  // 确保不超出屏幕范围
  int copy_w = (x + w <= screen_w) ? w : screen_w - x;
  int copy_h = (y + h <= screen_h) ? h : screen_h - y;

  // 逐行复制像素数据到帧缓冲
  for (int j = 0; j < copy_h; j++) {
    for (int i = 0; i < copy_w; i++) {
      fb[(y + j) * screen_w + (x + i)] = pixels[j * w + i];
    }
  }

  // 如果需要同步，向同步寄存器写入1
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
