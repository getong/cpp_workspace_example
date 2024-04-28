#include <stdio.h>

typedef int (*func_t)(int a, int b);

int sum(int a, int b) { return a + b; }

int main(void) {
  func_t fp = sum;
  printf ("%d\n", fp(1,2)) ;
  return 0;
}

// clang typedef_func_pointer.c
// gcc typedef_func_pointer.c