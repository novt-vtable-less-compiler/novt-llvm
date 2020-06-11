#include "test27.h"


class Z {
public:
	int z;
	int z2;
};
class Y {
public:
	int y;
};



class D : public virtual A, public C {
public:
	int d;
	Y& foo(Z& z) override {
		auto y = new Y();
		y->y = z.z + 4;
		return *y;
	}
};



Y& A::foo(Z& z) {
	auto y = new Y();
	y->y = z.z + 1;
	return *y;
}

Y& B::foo(Z& z) {
	auto y = new Y();
	y->y = z.z + 2;
	return *y;
}

A* get_some_class(int i) {
	if (i == 0)
		return new A();
	if (i == 1)
		return new B();
	if (i == 2)
		return new C();
	return new D();
}

void dispatch(A* a, Y& (A::*test)(Z&)) {
	Z z;
	z.z = 100;
	auto y = (a->*test)(z);
	printf("Dispatch on %p %d => %d\n", a, z.z, y.y);
}
