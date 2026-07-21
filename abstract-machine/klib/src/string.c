#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while(*s++) len++;
  return len;
}

char *strcpy(char *dst, const char *src) {
  // dst and src must not overlap .
  while(*src) {
    *dst++ = *src++;
  }
  *dst = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  int i = 0;
  for (i = 0; i < n && *src; i++) {
    *dst++ = *src++;
  }
  for (; i < n; i++) {
    *dst++ = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *p = dst;
  while(*p) p++;
  while(*src) {
    *p++ = *src++;
  }
  *p = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  int i = 0;
  for (i = 0; i < n && *s1 && *s2; i++) {
    if (*s1 != *s2) {
      return *s1 - *s2;
    }
    s1++;
    s2++;
  }
  if (i == n) return 0;
  return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
  // fill the first n bytes of the memory area pointed to by s with the constant byte c 
  char *p = s;
  char ch = (char)c;
  for (size_t i = 0; i < n; i++) {
    *p++ = ch;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  // maybe overlap !
  // if dst <= src, then copy from front to back 
  if (dst <= src) {
    for (int i = 0; i < n; i++) {
      ((char*)dst)[i] = ((char*)src)[i];
    }
  } 
  else if (dst >= src + n) {
    // still no overlap, copy from front to back
    for (int i = 0; i < n; i++) {
      ((char*)dst)[i] = ((char*)src)[i];
    }
  } 
  else {
    // overlap, firstly copy to a temporary buffer, then copy to dst 
    char tmp[n];
    for (int i = 0; i < n; i++) {
      tmp[i] = ((char*)src)[i];
    }
    for (int i = 0; i < n; i++) {
      ((char*)dst)[i] = tmp[i];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  // like memmove, but memcpy must not overlap .
  for (size_t i = 0; i < n; i++) {
    ((char*)out)[i] = ((char*)in)[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  for (int i = 0; i < n; i++) {
    if (((char*)s1)[i] != ((char*)s2)[i]) {
      return ((char*)s1)[i] - ((char*)s2)[i];
    }
  }
  return 0;
}

#endif
