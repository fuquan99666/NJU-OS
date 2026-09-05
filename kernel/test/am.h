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


// Arch-dependent processor context
typedef struct Context Context;

// An event of type @event, caused by @cause of pointer @ref
typedef struct {
  enum {
    EVENT_NULL = 0,
    EVENT_YIELD, EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR,
    EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
  } event;
  uintptr_t cause, ref;
  const char *msg;
} Event;

extern Area heap;
extern int heap_allocated;

// test中的am.h就包含test运行必须的函数即可

// cpu 总数， 由于test是直接本地运行，且我的电脑是16个物理核，每个可以2线程，所以返回16
static inline int cpu_count() {
  return 16;
}

static inline int cpu_current() {
  
}

static inline int atomic_xchg(volatile int *addr, int newval) {
  int result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "memory");
  return result;
}

#endif
