#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>


class A {
public:
    virtual ~A();

    virtual int foo(long x);

    virtual long long g(long x);

    virtual bool h(std::string &a, std::vector<std::string> &vec, long y, float z);
};

class B : public A {
public:
    virtual ~B();

    int foo(long x) override;
};

class C : public A {
public:
    virtual ~C();

    bool h(std::string &a, std::vector<std::string> &vec, long y, float z) override;
};


A *get_A();
B *get_B();
C *get_C();


int main(int argc, const char *argv[]);
