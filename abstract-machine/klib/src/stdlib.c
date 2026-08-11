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
  if (base != 10 && base != 16) {
    panic("itoa only support base 10 and 16");
  }

  if (base == 10) {

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
  } else if (base == 16) {
    // convert a number to a string in hex format 
    char *p = str;
    *p++ = '0';
    *p++ = 'x';
    char *start = p;
    
    while(true) {
      int digit = value % 16;
      if (digit < 10) {
        *p++ = '0' + digit;
      } else {
        *p++ = 'a' + digit - 10;
      }
      value /= 16;
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
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  #if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
    panic("Not implemented");
  #endif
    return NULL;

  // directly allocate memory from heap, which is a constant memory area .
  //heap_allocated += size;
  //if (heap_allocated > heap.end - heap.start) {
    //#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__)) 
      //panic("Out of memory");
    //#endif 
      //return NULL;
  //}

  

  //return (void *)(heap.start + heap_allocated - size);
}

void free(void *ptr) {
  // do nothing 
  // maybe later we can implement a free list to reuse the memory, but for now we just ignore it.
  return;
}

#endif
