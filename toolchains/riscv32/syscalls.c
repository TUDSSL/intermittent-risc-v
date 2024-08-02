// See LICENSE for license details.

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <sys/signal.h>
#include "util.h"
#include "encoding.h"

#define SYS_write 64

// Enable waiting for syscall processing
//#define SYSCALL_FROMHOST_WAIT

#undef strcmp

__attribute__ ((used))
volatile uint32_t tohost;
__attribute__ ((used))
volatile uint32_t fromhost;

static uintptr_t syscall(uintptr_t which, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
  volatile uint32_t magic_mem[8] __attribute__((aligned(64)));
  magic_mem[0] = which;
  magic_mem[1] = arg0;
  magic_mem[2] = arg1;
  magic_mem[3] = arg2;
  __sync_synchronize();

  tohost = (uintptr_t)magic_mem;

#ifdef SYSCALL_FROMHOST_WAIT
  while (fromhost == 0)
    ;
  fromhost = 0;
#endif

  __sync_synchronize();
  return magic_mem[0];
}

__attribute__((noreturn))
__attribute__((noinline))
void tohost_exit(uintptr_t code)
{
  tohost = (code << 1) | 1;
  while (1);
}

uintptr_t __attribute__((weak)) handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32])
{
  tohost_exit(1337);
}

void exit(int code)
{
  tohost_exit(code);
}

void abort()
{
  exit(128 + SIGABRT);
}

void printstr(const char* str)
{
  syscall(SYS_write, 1, (uintptr_t)str, strlen(str));
}

int main(void);

__attribute ((naked))
void _start(void) {
  __asm(""
  "li  x1, 0\n"
  "li  x2, 0\n"
  "li  x3, 0\n"
  "li  x4, 0\n"
  "li  x5, 0\n"
  "li  x6, 0\n"
  "li  x7, 0\n"
  "li  x8, 0\n"
  "li  x9, 0\n"
  "li  x10,0\n"
  "li  x11,0\n"
  "li  x12,0\n"
  "li  x13,0\n"
  "li  x14,0\n"
  "li  x15,0\n"
  "li  x16,0\n"
  "li  x17,0\n"
  "li  x18,0\n"
  "li  x19,0\n"
  "li  x20,0\n"
  "li  x21,0\n"
  "li  x22,0\n"
  "li  x23,0\n"
  "li  x24,0\n"
  "li  x25,0\n"
  "li  x26,0\n"
  "li  x27,0\n"
  "li  x28,0\n"
  "li  x29,0\n"
  "li  x30,0\n"
  "li  x31,0\n"

  //"li t0, MSTATUS_FS | MSTATUS_XS\n"
  //"csrs mstatus, t0\n"

  //"la t0, trap_entry\n"
  //"csrw mtvec, t0\n"

  ".option push\n"
  ".option norelax\n"
  "la gp, __global_pointer$\n"
  ".option pop\n"

  "la  tp, _end + 63\n"
  "and tp, tp, -64\n"

  "csrr a0, mhartid\n"
  "li a1, 1\n"
  "1:li t6, 5\n"
  "bgeu a0, a1, 1b\n"
  "li t6, 5\n"

  "add sp, a0, 1\n"
  "sll sp, sp, 17\n"
  "add sp, sp, tp\n"
  "sll a2, a0, 17\n"
  "add tp, tp, a2\n"

  "j _init\n"
  );
}

void _init(int cid, int nc)
{
  // only single-threaded programs should ever get here.
  int ret = main();
  exit(ret);
}


#define BUFFER_SYSCALL_PRINT
int putchar(int ch)
{
#ifdef BUFFER_SYSCALL_PRINT
  static char buf[64] __attribute__((aligned(64)));
  static int buflen = 0;

  buf[buflen++] = ch;

  if (ch == '\n' || buflen == sizeof(buf))
  {
    syscall(SYS_write, 1, (uintptr_t)buf, buflen);
    buflen = 0;
  }
#else
  syscall(SYS_write, 1, (uintptr_t)&ch, 1);
#endif

  return ch;
}

int puts(const char *str) {
  for (int i=0; i<strlen(str); i++) {
    putchar(str[i]);
  }
  putchar('\n');

  return 0;
}

void* memcpy(void* dest, const void* src, size_t len)
{
  if ((((uintptr_t)dest | (uintptr_t)src | len) & (sizeof(uintptr_t)-1)) == 0) {
    const uintptr_t* s = src;
    uintptr_t *d = dest;
    while (d < (uintptr_t*)(dest + len))
      *d++ = *s++;
  } else {
    const char* s = src;
    char *d = dest;
    while (d < (char*)(dest + len))
      *d++ = *s++;
  }
  return dest;
}

void* memset(void* dest, int byte, size_t len)
{
  if ((((uintptr_t)dest | len) & (sizeof(uintptr_t)-1)) == 0) {
    uintptr_t word = byte & 0xFF;
    word |= word << 8;
    word |= word << 16;
    word |= word << 16 << 16;

    uintptr_t *d = dest;
    while (d < (uintptr_t*)(dest + len))
      *d++ = word;
  } else {
    char *d = dest;
    while (d < (char*)(dest + len))
      *d++ = byte;
  }
  return dest;
}

int memcmp(const void *m1, const void *m2, size_t n) {
  unsigned char *s1 = (unsigned char *) m1;
  unsigned char *s2 = (unsigned char *) m2;

  while (n--)
    {
      if (*s1 != *s2)
	{
	  return *s1 - *s2;
	}
      s1++;
      s2++;
    }
  return 0;
}

size_t strlen(const char *s)
{
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

size_t strnlen(const char *s, size_t n)
{
  const char *p = s;
  while (n-- && *p)
    p++;
  return p - s;
}

int strcmp(const char* s1, const char* s2)
{
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
  } while (c1 != 0 && c1 == c2);

  return c1 - c2;
}

char* strcpy(char* dest, const char* src)
{
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}
