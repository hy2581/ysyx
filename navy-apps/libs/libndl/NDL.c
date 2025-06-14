#include <assert.h>
#include <fcntl.h> // 添加这一行，包含open()函数的声明
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0; // 屏幕大小
static int canvas_w = 0, canvas_h = 0; // 画布大小
static int canvas_x = 0, canvas_y = 0; // 相对于屏幕左上角的画布位置坐标

static struct timeval boot_time; // 存储初始时间

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }

  // 记录初始时间
  gettimeofday(&boot_time, NULL);
  return 0;
}

uint32_t NDL_GetTicks() {
  struct timeval now;
  gettimeofday(&now, NULL);

  // 计算相对于启动时间的毫秒数
  uint32_t ms = (now.tv_sec - boot_time.tv_sec) * 1000 +
                (now.tv_usec - boot_time.tv_usec) / 1000;
  return ms;
}

int NDL_PollEvent(char *buf, int len) {
  int fd = open("/dev/events", 0, 0);
  int ret = read(fd, buf, len);
  assert(close(fd) == 0);
  return ret == 0 ? 0 : 1;
}

void NDL_OpenCanvas(int *w, int *h) {
  // 添加调试输出
  printf("NDL_OpenCanvas: 请求画布大小 %d x %d\n", *w, *h);

  if (getenv("NWM_APP")) {
    // 不要修改NWM_APP相关的代码
    return;
  }

  // 读取屏幕信息
  int buf_size = 1024;
  char *buf = (char *)malloc(buf_size * sizeof(char));
  int fd = open("/proc/dispinfo", O_RDONLY);
  int ret = read(fd, buf, buf_size);
  assert(ret < buf_size); // 确保读取完整
  assert(close(fd) == 0);

  int i = 0;
  int width = 0, height = 0;

  // 解析WIDTH
  assert(strncmp(buf + i, "WIDTH", 5) == 0);
  i += 5;
  for (; i < buf_size; ++i) {
    if (buf[i] == ':') {
      i++;
      break;
    }
    assert(buf[i] == ' ');
  }
  for (; i < buf_size; ++i) {
    if (buf[i] >= '0' && buf[i] <= '9')
      break;
    assert(buf[i] == ' ');
  }
  for (; i < buf_size; ++i) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      width = width * 10 + buf[i] - '0';
    } else {
      break;
    }
  }
  assert(buf[i++] == '\n');

  // 解析HEIGHT
  assert(strncmp(buf + i, "HEIGHT", 6) == 0);
  i += 6;
  for (; i < buf_size; ++i) {
    if (buf[i] == ':') {
      i++;
      break;
    }
    assert(buf[i] == ' ');
  }
  for (; i < buf_size; ++i) {
    if (buf[i] >= '0' && buf[i] <= '9')
      break;
    assert(buf[i] == ' ');
  }
  for (; i < buf_size; ++i) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      height = height * 10 + buf[i] - '0';
    } else {
      break;
    }
  }

  free(buf);

  // 设置屏幕大小
  screen_w = width;
  screen_h = height;

  // 设置画布大小和位置
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }
  canvas_w = *w;
  canvas_h = *h;
  canvas_x = (screen_w - canvas_w) / 2;
  canvas_y = (screen_h - canvas_h) / 2;

  printf("NDL_OpenCanvas: 屏幕大小 %d x %d, 画布大小 %d x %d, 位置 (%d, %d)\n",
         screen_w, screen_h, canvas_w, canvas_h, canvas_x, canvas_y);
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  int fd = open("/dev/fb", O_WRONLY);
  if (fd < 0) {
    printf("NDL_DrawRect: 打开文件失败!\n");
    return;
  }

  // 计算画布在屏幕上的实际坐标
  int screen_x = x + canvas_x;
  int screen_y = y + canvas_y;

  printf("NDL_DrawRect: 屏幕位置 (%d, %d)\n", screen_x, screen_y);

  for (int i = 0; i < h; i++) {
    // 计算每行的偏移量 (以字节为单位)
    size_t offset = ((screen_y + i) * screen_w + screen_x) * sizeof(uint32_t);

    // 打印实际使用的偏移量进行调试
    printf("NDL_DrawRect: 行%d, 偏移量 %d 字节\n", i, offset);

    // 设置文件位置
    off_t result = lseek(fd, offset, SEEK_SET);
    if (result == -1) {
      printf("NDL_DrawRect: lseek失败，行%d\n", i);
      continue;
    }

    // 写入一行像素数据
    size_t len = w * sizeof(uint32_t);
    ssize_t written = write(fd, pixels + i * w, len);
    if (written != len) {
      printf("NDL_DrawRect: write不完整，写入%d/%d字节\n", written, len);
    }
  }

  close(fd);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

void NDL_Quit() {
}
