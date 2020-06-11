#include "test28.h"
#include <stdio.h>

namespace {
	class B : public A {
		long y;
	public:
		~B() override {
			printf("  [~B v1 %p]", this);
		}

		void f(int x) override {
			printf("B::f(%d) this=%p v1\n", x, this);
		}
	};
};

A* getA1() {
	return new A();
}

A* getB1() {
	return new B();
}
