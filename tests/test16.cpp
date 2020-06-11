#include <stdio.h>


class A {
public:
	long a = 10;
	long b = 20;
	long c = 30;

	virtual ~A() = default;

	virtual long f() {
		return 10000*a + 100*b + c;
	}

	long g() {
		return c+1;
	}

};

class B : public A {
	long d = 50;
public:
	B() {
		c = 40;
	}

	virtual ~B() = default;

	long f() override {
		return 10000*a + 100*b + c + 10203;
	}
};


typedef long (A::*MemberFunctionPtr)();



long __attribute__ ((noinline))  dispatch(A& object, MemberFunctionPtr function) {
	return (object.*function)();
}



int main() {
	A* a = new A();
	A* b = new B();

	auto funcA = &A::f;
	auto funcG = &A::g;

	printf("a funcA : %ld == 102030\n", dispatch(*a, funcA));
	printf("b funcA : %ld == 112243\n", dispatch(*b, funcA));
	printf("a funcG :     %ld == 31\n", dispatch(*a, funcG));
	printf("b funcG :     %ld == 41\n", dispatch(*b, funcG));

	return 0;
}
