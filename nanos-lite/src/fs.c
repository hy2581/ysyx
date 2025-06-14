#include <fs.h>
#include <string.h> // 添加这一行

// 添加ramdisk函数的声明
extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

// 添加函数声明
extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset,
                       size_t len); // 添加这一行
extern size_t dispinfo_read(void *buf, size_t offset, size_t len); // 添加这一行

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

// 定义无效读写函数（用于不支持的操作）
size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("invalid read");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("invalid write");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
    // 标准输入：设置读写都为无效操作
    [FD_STDIN] = {"stdin", 0, 0, invalid_read, invalid_write},
    // 标准输出：读取无效，写入指向串口
    [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
    // 标准错误：读取无效，写入指向串口
    [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
    // 事件设备：支持读取，不支持写入
    [FD_FB] = {"/dev/events", 0, 0, events_read, invalid_write},
    // 显示设备：支持写入，不支持读取
    {"/dev/fb", 0, 0, invalid_read, fb_write},
    // 显示信息：支持读取，不支持写入
    {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

// 文件当前偏移量
static size_t file_open_offset[NR_FILES];

int fs_open(const char *pathname, int flags, int mode) {
  // printf("Trying to open file: %s\n", pathname);

  // 检查是否是绝对路径
  const char *real_path = pathname;

  // 如果是以 /home 开头的绝对路径，尝试查找 /share/ 或 /bin/
  if (strncmp(pathname, "/home/", 6) == 0) {
    // 手动查找 /share/ 或 /bin/
    const char *p = pathname;
    while (*p) {
      if (strncmp(p, "/share/", 7) == 0) {
        real_path = p;
        break;
      }
      if (strncmp(p, "/bin/", 5) == 0) {
        real_path = p;
        break;
      }
      p++;
    }

    if (real_path != pathname) {
      printf("Converting absolute path to: %s\n", real_path);
    }
  }

  // 剩余代码保持不变
  // 遍历文件表，查找匹配的文件名
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(file_table[i].name, real_path) == 0) {
      // 找到文件，初始化偏移量为0
      file_open_offset[i] = 0;
      // printf("File found! FD = %d\n", i);
      return i; // 返回文件描述符（即文件表索引）
    }
  }

  // 未找到文件，报错
  panic("File %s not found!", real_path);
  return -1; // 不会执行到这里
}

size_t fs_read(int fd, void *buf, size_t len) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);

  // 获取文件信息
  Finfo *file = &file_table[fd];

  // 首先检查是否有专门的读函数
  ReadFn readFn = file->read;
  if (readFn != NULL) {
    // 对于设备文件，忽略偏移量
    return readFn(buf, 0, len);
  }

  // 以下是原来的代码，处理普通文件
  size_t offset = file_open_offset[fd];

  // 检查是否超出文件末尾
  if (offset >= file->size) {
    return 0; // 已到达文件末尾，无数据可读
  }

  // 计算实际可读取的字节数
  size_t real_len = (offset + len <= file->size) ? len : (file->size - offset);

  // 特殊文件处理
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR) {
    // 对于stdin可以实现从控制台读取，这里简化处理
    return 0;
  }

  // 计算绝对偏移量
  size_t absolute_offset = file->disk_offset + offset;

  // 在计算和调用之间不要插入其他代码
  printf("[fs_read] 准备调用ramdisk_read: fd=%d, offset=%d, len=%d\n", fd,
         (int)absolute_offset, (int)real_len);

  // 立即调用，不要在计算和调用之间插入代码
  size_t result = ramdisk_read(buf, absolute_offset, real_len);

  printf("[fs_read] ramdisk_read返回: %d\n", (unsigned long)result);

  // 更新文件偏移量
  file_open_offset[fd] += real_len;

  return real_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);

  // 获取文件信息
  Finfo *file = &file_table[fd];

  // 首先检查是否有专门的写函数
  WriteFn writeFn = file->write;
  if (writeFn != NULL) {
    // 传递当前文件的偏移量，而不是固定的0
    return writeFn(buf, file_open_offset[fd], len);
  }

  // 以下是原来的代码，处理普通文件
  size_t offset = file_open_offset[fd];

  // 特殊文件处理
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    // 标准输出或错误：输出到控制台
    const char *cbuf = (const char *)buf;
    for (size_t i = 0; i < len; i++) {
      putch(cbuf[i]);
    }
    return len;
  }

  if (fd == FD_STDIN) {
    // 不能写入标准输入
    return 0;
  }

  // 检查是否超出文件大小
  if (offset >= file->size) {
    return 0; // 已到达文件末尾，无法写入
  }

  // 计算实际可写入的字节数
  size_t real_len = (offset + len <= file->size) ? len : (file->size - offset);

  // 写入ramdisk
  ramdisk_write(buf, file->disk_offset + offset, real_len);

  // 更新文件偏移量
  file_open_offset[fd] += real_len;

  return real_len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  printf("fs_lseek: fd=%d, offset=%d, whence=%d\n", fd, offset, whence);

  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid fd in fs_lseek: %d", fd);
    return -1;
  }

  size_t new_offset = 0;

  switch (whence) {
  case SEEK_SET:
    new_offset = offset;
    break;
  case SEEK_CUR:
    new_offset = file_open_offset[fd] + offset;
    break;
  case SEEK_END:
    new_offset = file_table[fd].size + offset;
    break;
  default:
    panic("Invalid whence in fs_lseek: %d", whence);
    return -1;
  }

  if (new_offset > file_table[fd].size && fd != FD_FB) {
    panic("Offset too large in fs_lseek: %d > %d", new_offset,
          file_table[fd].size);
    return -1;
  }

  printf("fs_lseek: 设置fd=%d的偏移量为%d\n", fd, new_offset);
  file_open_offset[fd] = new_offset;
  return new_offset;
}

int fs_close(int fd) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);

  // 在我们的简易文件系统中，不需要特别的关闭操作
  // 只需确保下次打开时偏移量重置
  file_open_offset[fd] = 0;

  return 0; // 总是成功关闭
}

void init_fs() {
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;

  printf("init_fs: 初始化屏幕大小 %d x %d\n", width, height);

  // 查找并设置/dev/fb文件大小
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(file_table[i].name, "/dev/fb") == 0) {
      file_table[i].size = width * height * sizeof(uint32_t);
      printf("init_fs: 设置/dev/fb大小为 %d 字节\n", file_table[i].size);
      break;
    }
  }

  // 初始化偏移量
  for (int i = 0; i < NR_FILES; i++) {
    file_open_offset[i] = 0;
  }
}

int sys_write(int fd, const void *buf, size_t count) {
  // 不再判断fd是否为1或2，直接通过fs_write调用对应设备的写函数
  return fs_write(fd, buf, count);
}
