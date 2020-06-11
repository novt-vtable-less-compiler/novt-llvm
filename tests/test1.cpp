#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
void __on_error(long long x) {
  printf("!!! ERROR !!!\n code = %lld\n", x);
  exit(1);
}
}



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

void test(A *a, B *b) {
  long x = 27;
  printf("1: a->foo(x)   = %d\n", a->foo(x));
  printf("2: a->g(x)     = %lld\n", a->g(x));
  printf("3: b->foo(x+1) = %d\n", b->foo(x + 1));
}

int main(int argc, const char *argv[]) {
  if (argc == 1) __on_error(-1);
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
