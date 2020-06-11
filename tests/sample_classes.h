#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>


class A {
public:
    virtual ~A(){
      puts("delete A");
    }

    virtual int foo(long x);

    virtual long long g(long x);

    virtual bool h(std::string &a, std::vector<std::string> &vec, long y, float z);
};

class B : public A {
public:
    virtual ~B(){
      puts("delete B");
    }

    int foo(long x) override;
};

class C : public A {
public:
    virtual ~C(){
      puts("delete C");
    }

    bool h(std::string &a, std::vector<std::string> &vec, long y, float z) override;
};


int A::foo(long x) { return x + 1; }

int B::foo(long x) { return x + 2; }

long long A::g(long x) { return x + 3; }

bool A::h(std::string &a, std::vector<std::string> &vec, long y, float z) { return y == 0; }

bool C::h(std::string &a, std::vector<std::string> &vec, long y, float z) { return y == 1; }


A *get_A() {
  return new A();
}

B *get_B() {
  return new B();
}

C *get_C() {
  return new C();
}

A *get(const char* input) {
  A *a = input[0] == 'a' ? get_A() : input[0] == 'b' ? (A *) get_B() : (A *) get_C();
  return a;
}
