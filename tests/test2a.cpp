#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "test2.hpp"

A::~A(){
  puts("delete A");
}

int A::foo(long x) { return x + 1; }

long long A::g(long x) { return x + 3; }

bool A::h(std::string &a, std::vector<std::string> &vec, long y, float z) { return y == 0; }

A *get_A() {
  return new A();
}
