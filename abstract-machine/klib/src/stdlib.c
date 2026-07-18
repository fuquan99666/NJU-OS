#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void itoa(int value, char *str, int base) {
  // now only support base 10
  if (base != 10) {
    panic("itoa only support base 10");
  }
  char *p = str;
  if (value < 0) {
    *p++ = '-';
    value = -value;
  }
  char *start = p;
  
  while(true) {
    *p++ = '0' + value % 10;
    value /= 10;
    if (value == 0) {
      break;
    }
  }
  *p = '\0';
  // reverse the string
  char *end = p - 1;
  while (start < end) {
    char tmp = *start; 
    *start++ = *end;
    *end-- = tmp;
  }
  
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  // #if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  //   panic("Not implemented");
  // #endif
  //   return NULL;

  // directly allocate memory from heap, which is a constant memory area .
}

void free(void *ptr) {
}

#endif
