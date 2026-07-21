#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int printf_lock = 0;
static int vsprintf_lock = 0;
static int sprintf_lock = 0;
static int snprintf_lock = 0;
static int vsnprintf_lock = 0;

int printf(const char *fmt, ...) {

  // We must use stdarg to parse the variable arguments
  // because different arch may have different calling conventions for variable arguments.

  // and We should use a lock to avoid multiple threads printing at the same time.

  // try to acquire the lock 
  while(atomic_xchg(&printf_lock, 1)) {
    // busy wait
  }

  assert(printf_lock == 1); // we have acquired the lock 
  
  va_list ap;
  int d;
  char c;
  char *s;

  va_start(ap, fmt);
  while(*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt++)
      {
      case 'd':
        d = va_arg(ap, int);
        char num[32];
        itoa(d, num, 10);
        putstr(num);

        break;

      case 'c':
        c = (char)va_arg(ap, int);
        putch(c);

        break;

      case 's':
        s = va_arg(ap, char *);
        putstr(s);

        break;
      
      default:
        assert(0);
      }
    } else {
      putch(*fmt++);
    }
  }

  va_end(ap);

  // release the lock 
  atomic_xchg(&printf_lock, 0);
  return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  // vsprintf is similar to vsnprintf, but it does not have a size limit for the output buffer.

  // try to acquire the lock
  while(atomic_xchg(&vsprintf_lock, 1)) {
    // busy wait
  }

  assert(vsprintf_lock == 1); // we have acquired the lock

  int d;
  char c;
  char *s;

  while(*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt++)
      {
      case 'd':
        d = va_arg(ap, int);
        char num[32];
        itoa(d, num, 10);
        for (char *p = num; *p; p++) {
          *out++ = *p;
        }
        break;
      case 'c':
        c = (char)va_arg(ap, int);
        *out++ = c;
        break;
      case 's':
        s = va_arg(ap, char *);
        for (char *p = s; *p; p++) {
          *out++ = *p;
        }
        break;
      default:
        assert(0);
      }
    }
  }

  // release the lock
  atomic_xchg(&vsprintf_lock, 0);

  return 0;
}

int sprintf(char *out, const char *fmt, ...) {
  // sprintf is similar to printf, but it writes the output to a string instead of the console. 

  // try to acquire the lock
  while(atomic_xchg(&sprintf_lock, 1)) {
    // busy wait
  }
  assert(sprintf_lock == 1); // we have acquired the lock

  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);

  assert(ret >= 0); // vsprintf should not fail

  // release the lock
  atomic_xchg(&sprintf_lock, 0);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  // snprintf is similar to sprintf, but it has a size limit for the output buffer.

  // try to acquire the lock
  while(atomic_xchg(&snprintf_lock, 1)) {
    // busy wait
  }

  assert(snprintf_lock == 1); // we have acquired the lock

  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);

  assert(0 <= ret && ret <= n); // vsnprintf should not fail

  // release the lock
  atomic_xchg(&snprintf_lock, 0);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  // vsnprintf is similar to snprintf, but it takes a va_list instead of a variable number of arguments.

  // try to acquire the lock
  while(atomic_xchg(&vsnprintf_lock, 1)) {
    // busy wait
  }

  assert(vsnprintf_lock == 1); // we have acquired the lock

  int ret = vsprintf(out, fmt, ap);

  assert(0 <= ret && ret <= n); // vsnprintf should not fail
  // release the lock
  atomic_xchg(&vsnprintf_lock, 0);
  return ret;
}

#endif
