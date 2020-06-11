#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "test2.hpp"

C::~C(){
  puts("delete C");
}

bool C::h(std::string &a, std::vector<std::string> &vec, long y, float z) { return y == 1; }


C *get_C() {
  return new C();
}

