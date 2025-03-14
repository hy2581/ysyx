#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  // 分配足够大的缓冲区用于格式化输出
  char buf[4096];
  va_list ap;
  int ret;
  
  // 使用vsprintf将格式化后的内容写入缓冲区
  va_start(ap, fmt);
  ret = vsprintf(buf, fmt, ap);
  va_end(ap);
  
  // 将缓冲区内容逐字符输出到标准输出
  char *p = buf;
  while (*p) {
    putch(*p++);
  }
  
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *original_out = out;
  char c;
  
  while ((c = *fmt++)) {
    if (c != '%') {
      *out++ = c;
      continue;
    }
    
    // 处理格式说明符
    c = *fmt++;
    if (c == '\0') break; // 防止格式字符串以%结尾
    
    switch (c) {
      case 's': {  // 字符串
        char *s = va_arg(ap, char *);
        if (!s) s = "(null)";
        while (*s) {
          *out++ = *s++;
        }
        break;
      }
      case 'd': case 'i': {  // 有符号整数
        int num = va_arg(ap, int);
        if (num < 0) {
          *out++ = '-';
          num = -num;
        }
        
        // 如果是0直接输出
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算数字的位数
        char temp[16];
        int i = 0;
        while (num > 0) {
          temp[i++] = '0' + (num % 10);
          num /= 10;
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case 'x': case 'X': {  // 十六进制整数
        unsigned int num = va_arg(ap, unsigned int);
        // 如果是0直接输出
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算数字的位数
        char temp[16];
        int i = 0;
        while (num > 0) {
          int digit = num % 16;
          if (digit < 10)
            temp[i++] = '0' + digit;
          else
            temp[i++] = (c == 'x' ? 'a' : 'A') + (digit - 10);
          num /= 16;
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case 'c': {  // 字符
        char ch = (char)va_arg(ap, int);
        *out++ = ch;
        break;
      }
      case 'u': {  // 无符号整数
        unsigned int num = va_arg(ap, unsigned int);
        
        // 如果是0直接输出
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算数字的位数
        char temp[16];
        int i = 0;
        while (num > 0) {
          temp[i++] = '0' + (num % 10);
          num /= 10;
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case 'p': {  // 指针
        *out++ = '0';
        *out++ = 'x';
        uintptr_t num = (uintptr_t)va_arg(ap, void *);
        
        // 如果是0直接输出
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算数字的位数
        char temp[16];
        int i = 0;
        while (num > 0) {
          int digit = num % 16;
          if (digit < 10)
            temp[i++] = '0' + digit;
          else
            temp[i++] = 'a' + (digit - 10);
          num /= 16;
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case '%': {  // 百分号
        *out++ = '%';
        break;
      }
      default:
        *out++ = c;
        break;
    }
  }
  
  *out = '\0';  // 添加字符串结束符
  return out - original_out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  int ret;
  
  va_start(ap, fmt);
  ret = vsprintf(out, fmt, ap);
  va_end(ap);
  
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif