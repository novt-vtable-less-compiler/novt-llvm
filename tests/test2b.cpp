#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "test2.hpp"

B::~B(){
  puts("delete B");
}

int B::foo(long x) { return x + 2; }

B *get_B() {
  return new B();
}
