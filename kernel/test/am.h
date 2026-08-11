#ifndef TEST_AM_H__
#define TEST_AM_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define Bug() printf("HaHa\n")

typedef struct {
	void *start;
	void *end;
} Area;

extern Area heap;
extern int heap_allocated;

static inline int atomic_xchg(volatile int *addr, int newval) {
  int result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "memory");
  return result;
}

#endif
