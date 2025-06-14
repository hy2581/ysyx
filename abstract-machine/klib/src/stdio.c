#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  // 分配足够大的缓冲区用于格式化输出
  char buf[4096];
  va_list ap;//用于访问可变参数
  int ret;
  
  // 使用vsprintf将格式化后的内容写入缓冲区
  va_start(ap, fmt);
  /*
  第一个参数是 va_list 变量
  第二个参数 fmt 是可变参数列表前的最后一个固定参数，即格式字符串
  */
  ret = vsprintf(buf, fmt, ap);
  va_end(ap);
  
  // 将缓冲区内容逐字符输出到标准输出
  putstr(buf);

  return ret;
}
/*
vsprintf 是一个格式化输出函数，它的作用是根据格式字符串将数据格式化后写入到字符串缓冲区中。
out: 输出缓冲区，用于存储格式化后的字符串
fmt: 格式字符串，包含普通文本和格式说明符(如 %d、%s 等)
ap: va_list 类型的可变参数列表，包含与格式说明符对应的参数值
返回值: 写入到输出缓冲区的字符数（不包括结尾的'\0'字符）
*/
int vsprintf(char *out, const char *fmt, va_list ap) {
  // 保存原始输出缓冲区的起始位置，用于最后计算写入的字符数
  char *original_out = out;
  char c;  // 用于存储当前处理的格式字符串中的字符
  
  // 循环遍历格式字符串中的每个字符
  while ((c = *fmt++)) {  // 获取当前字符并将格式字符串指针向后移动
    if (c != '%') {  // 如果不是格式说明符的开始符号'%'
      *out++ = c;    // 直接将该字符复制到输出缓冲区，并移动输出指针
      continue;      // 继续处理下一个字符
    }
    
    // 处理格式说明符 - 已经确认遇到了'%'
    c = *fmt++;      // 获取'%'后面的字符（格式类型）并移动指针
    if (c == '\0') break; // 防止格式字符串以'%'结尾导致越界访问
    
    // 根据格式类型字符处理不同的数据格式
    switch (c) {
      case 's': {  // 字符串格式：%s
        // 从参数列表获取一个字符串指针
        char *s = va_arg(ap, char *);
        if (!s) s = "(null)";  // 如果指针为NULL，显示"(null)"
        // 将字符串内容复制到输出缓冲区
        while (*s) {
          *out++ = *s++;
        }
        break;
      }
      case 'd': case 'i': {  // 有符号整数格式：%d 或 %i
        // 从参数列表获取一个整数
        int num = va_arg(ap, int);
        if (num < 0) {  // 处理负数
          *out++ = '-';  // 添加负号
          num = -num;    // 转为正数处理
        }
        
        // 如果是0直接输出字符'0'
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算数字的各个位，并存储到临时数组（注意：是逆序存储的）
        char temp[16];  // 临时存储转换后的数字字符
        int i = 0;
        while (num > 0) {
          temp[i++] = '0' + (num % 10);  // 将个位数字转为字符并存储
          num /= 10;  // 去掉个位数字
        }
        
        // 逆序输出（因为之前是从低位到高位存储的）
        while (i > 0) {
          *out++ = temp[--i];  // 从高位到低位输出
        }
        break;
      }
      case 'x': case 'X': {  // 十六进制整数格式：%x（小写）或 %X（大写）
        unsigned int num = va_arg(ap, unsigned int);
        // 如果是0直接输出
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算十六进制数字的各位
        char temp[16];
        int i = 0;
        while (num > 0) {
          int digit = num % 16;  // 取16的余数（0-15之间）
          if (digit < 10)
            temp[i++] = '0' + digit;  // 0-9用数字字符表示
          else
            // 10-15用a-f（小写）或A-F（大写）表示
            temp[i++] = (c == 'x' ? 'a' : 'A') + (digit - 10);
          num /= 16;  // 去掉最低位
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case 'c': {  // 字符格式：%c
        // 获取一个字符参数（注意va_arg会将char提升为int传递）
        char ch = (char)va_arg(ap, int);
        *out++ = ch;  // 直接输出该字符
        break;
      }
      case 'u': {  // 无符号整数格式：%u
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
          temp[i++] = '0' + (num % 10);  // 转换为字符
          num /= 10;  // 去掉个位数字
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case 'p': {  // 指针格式：%p（输出为十六进制）
        *out++ = '0';  // 输出前缀"0x"
        *out++ = 'x';
        // 将指针转换为无符号整数类型
        uintptr_t num = (uintptr_t)va_arg(ap, void *);
        
        // 如果是0直接输出
        if (num == 0) {
          *out++ = '0';
          break;
        }
        
        // 计算十六进制表示的各位
        char temp[16];
        int i = 0;
        while (num > 0) {
          int digit = num % 16;
          if (digit < 10)
            temp[i++] = '0' + digit;  // 0-9用数字字符表示
          else
            temp[i++] = 'a' + (digit - 10);  // 10-15用a-f表示
          num /= 16;
        }
        
        // 逆序输出
        while (i > 0) {
          *out++ = temp[--i];
        }
        break;
      }
      case '%': {  // 百分号：%%（输出一个%字符）
        *out++ = '%';
        break;
      }
      default:  // 未知格式说明符，直接输出该字符
        *out++ = c;
        break;
    }
  }
  
  *out = '\0';  // 在输出缓冲区末尾添加字符串结束符（空字符）
  return out - original_out;  // 返回写入的字符数（不包括结尾的空字符）
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
  va_list ap;
  int ret;
  
  va_start(ap, fmt);
  ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  // 保存原始输出缓冲区指针，用于计算写入的字符数
  char *original_out = out;
  // 最大可写入字符数(为null终止符预留一个位置)
  size_t remaining = n > 0 ? n - 1 : 0;
  char c;
  
  // 如果n为0，仍然需要计算输出大小，但不写入任何字符
  bool no_output = (n == 0);
  
  // 循环遍历格式字符串中的每个字符
  while ((c = *fmt++)) {
    if (c != '%') {  // 普通字符
      if (!no_output && remaining > 0) {
        *out++ = c;
        remaining--;
      } else if (no_output) {
        // 虽然不输出，但仍需计数
        out++;
      }
      continue;
    }
    
    // 处理格式说明符
    c = *fmt++;
    if (c == '\0') break; // 防止格式字符串以'%'结尾导致越界访问
    
    // 根据格式类型字符处理不同的数据格式
    switch (c) {
      case 's': {  // 字符串格式：%s
        char *s = va_arg(ap, char *);
        if (!s) s = "(null)";
        
        while (*s) {
          if (!no_output && remaining > 0) {
            *out++ = *s++;
            remaining--;
          } else {
            out++;
            s++;
          }
        }
        break;
      }
      case 'd': case 'i': {  // 有符号整数格式
        int num = va_arg(ap, int);
        if (num < 0) {
          if (!no_output && remaining > 0) {
            *out++ = '-';
            remaining--;
          } else if (no_output) {
            out++;
          }
          num = -num;
        }
        
        // 如果是0直接输出字符'0'
        if (num == 0) {
          if (!no_output && remaining > 0) {
            *out++ = '0';
            remaining--;
          } else if (no_output) {
            out++;
          }
          break;
        }
        
        // 计算数字的各个位
        char temp[16];
        int i = 0;
        while (num > 0) {
          temp[i++] = '0' + (num % 10);
          num /= 10;
        }
        
        // 逆序输出
        while (i > 0) {
          if (!no_output && remaining > 0) {
            *out++ = temp[--i];
            remaining--;
          } else {
            out++;
            i--;
          }
        }
        break;
      }
      case 'x': case 'X': {  // 十六进制整数格式
        unsigned int num = va_arg(ap, unsigned int);
        if (num == 0) {
          if (!no_output && remaining > 0) {
            *out++ = '0';
            remaining--;
          } else if (no_output) {
            out++;
          }
          break;
        }
        
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
        
        while (i > 0) {
          if (!no_output && remaining > 0) {
            *out++ = temp[--i];
            remaining--;
          } else {
            out++;
            i--;
          }
        }
        break;
      }
      case 'c': {  // 字符格式
        char ch = (char)va_arg(ap, int);
        if (!no_output && remaining > 0) {
          *out++ = ch;
          remaining--;
        } else if (no_output) {
          out++;
        }
        break;
      }
      case 'u': {  // 无符号整数格式
        unsigned int num = va_arg(ap, unsigned int);
        if (num == 0) {
          if (!no_output && remaining > 0) {
            *out++ = '0';
            remaining--;
          } else if (no_output) {
            out++;
          }
          break;
        }
        
        char temp[16];
        int i = 0;
        while (num > 0) {
          temp[i++] = '0' + (num % 10);
          num /= 10;
        }
        
        while (i > 0) {
          if (!no_output && remaining > 0) {
            *out++ = temp[--i];
            remaining--;
          } else {
            out++;
            i--;
          }
        }
        break;
      }
      case 'p': {  // 指针格式
        if (!no_output && remaining > 0) {
          *out++ = '0';
          remaining--;
        } else if (no_output) {
          out++;
        }
        
        if (!no_output && remaining > 0) {
          *out++ = 'x';
          remaining--;
        } else if (no_output) {
          out++;
        }
        
        uintptr_t num = (uintptr_t)va_arg(ap, void *);
        if (num == 0) {
          if (!no_output && remaining > 0) {
            *out++ = '0';
            remaining--;
          } else if (no_output) {
            out++;
          }
          break;
        }
        
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
        
        while (i > 0) {
          if (!no_output && remaining > 0) {
            *out++ = temp[--i];
            remaining--;
          } else {
            out++;
            i--;
          }
        }
        break;
      }
      case '%': {  // 百分号
        if (!no_output && remaining > 0) {
          *out++ = '%';
          remaining--;
        } else if (no_output) {
          out++;
        }
        break;
      }
      default:  // 未知格式说明符
        if (!no_output && remaining > 0) {
          *out++ = c;
          remaining--;
        } else if (no_output) {
          out++;
        }
        break;
    }
  }
  
  // 添加字符串结束符
  if (!no_output && n > 0) {
    *out = '\0';
  }
  
  return out - original_out;  // 返回本应写入的字符数(不包括结尾的'\0')
}

#endif