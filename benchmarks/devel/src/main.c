#include <stdio.h>
#include <stdlib.h>

extern void rc_test();
extern void tohost_exit(uintptr_t code);

int main()
{
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;
  int e = 5;
  int f = 6;
  int g = 7;

  volatile int g1 = g;
  g += 849;
  g1 = g;

  volatile int a1 = a;
  volatile int b1 = b;
  volatile int c1 = c;
  volatile int d1 = d;
  volatile int e1 = e;
  volatile int f1 = f;
  volatile int a2 = a;
  volatile int b2 = b;
  volatile int c2 = c;
  volatile int d2 = d;
  volatile int e2 = e;
  volatile int f2 = f;

  a += 80;
  volatile int z = a;

  return 0;
}
