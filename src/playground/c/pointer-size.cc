
/*
 * Compares pointer sizes on 32 vs. 64 bit archs.
 *
 * Code as implemented below will only compile on 32-bit archs.
 *
 * g++ -Wall pointer-size.c
 * g++ -Wall -m32 pointer-size.c
 */
#include <stdio.h>
#include <string.h>

struct mystruct {
  int a;
  int b;
  int c;
  int d;
}; // 128bit

int main(int argc, char *argv[])
{
  mystruct *i = new mystruct;

  printf("Sizeof struct: %zu bytes\n", sizeof(*i));
  printf("Sizeof pointer: %zu bytes\n", sizeof(i));
  printf("Sizeof void ptr: %zu bytes\n", sizeof((void*)i));
  printf("Sizeof int cast ptr: %zu bytes\n", sizeof((int*)i));
  printf("Sizeof int: %zu\n", sizeof(int));

  int val = (int)((int*)i);
  mystruct *p = (mystruct*)val;

  printf("ptr equals? %p == %p\n", i, p);
}
