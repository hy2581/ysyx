#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len]) {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *orig = dst;
  while ((*dst++ = *src++));
  return orig;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *orig = dst;
  size_t i;
  
  // 复制源字符串内容，最多n个字符
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  
  // 如果源字符串长度小于n，用'\0'填充剩余空间
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  
  return orig;
}

char *strcat(char *dst, const char *src) {
  char *orig = dst;
  
  // 找到目标字符串的末尾
  while (*dst) {
    dst++;
  }
  
  // 复制源字符串到目标字符串末尾
  while ((*dst++ = *src++));
  
  return orig;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) {
    return 0;
  }
  
  while (n-- > 0 && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  
  return n < 0 ? 0 : *(unsigned char *)s1 - *(unsigned char *)s2;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  
  while (n--) {
    *p++ = (unsigned char)c;
  }
  
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  
  if (d == s) {
    return dst;
  }
  
  // 如果目标区域在源区域之后且有重叠，从后向前复制
  if (d > s && d < s + n) {
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  } else {
    // 否则从前向后复制
    while (n--) {
      *d++ = *s++;
    }
  }
  
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = (unsigned char *)out;
  const unsigned char *s = (const unsigned char *)in;
  
  // 简单地从前向后复制，不考虑重叠
  while (n--) {
    *d++ = *s++;
  }
  
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  
  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  
  return 0;
}

#endif