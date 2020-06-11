#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "test2.hpp"


void test(A *a, B *b) {
  long x = 27;
  printf("1: a->foo(x)   = %d\n", a->foo(x));
  printf("2: a->g(x)     = %lld\n", a->g(x));
  printf("3: b->foo(x+1) = %d\n", b->foo(x + 1));
}

int main(int argc, const char *argv[]) {
  if (argc == 1) {
    puts("!!! INVALID ARGUMENT !!!");
    exit(1);
  }
  puts("A");
  B b;
  //printf("B = %p\n", &b);
  A *a = argv[1][0] == 'a' ? get_A() : argv[1][0] == 'b' ? (A *) get_B() : (A *) get_C();
  //printf("C, a = %p\n", a);
  test(a, &b);
  puts("D");
  delete a;
  return 0;
}
