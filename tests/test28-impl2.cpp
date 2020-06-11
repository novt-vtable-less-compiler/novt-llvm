#include "test28.h"
#include <stdio.h>

namespace {
	class B : public A {
		long y;
		long y2;
	public:
		~B() override {
			printf("  [~B v2 %p]", this);
		}

		void f(int x) override {
			printf("B::f(%d) this=%p v2\n", x, this);
		}
	};

	class C : public B {
	public:
		~C() override {
			printf("  [~C %p]", this);
		}		
	};
};

A* getA2() {
	return new A();
}

A* getB2() {
	return new B();
}

A* getB3() {
	return new C();
}
