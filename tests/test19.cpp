#include <stdio.h>

// test dynamic_cast<void*>


class A {
public:
	int field;

	virtual int foo() {
		return field + 10;
	}
};

class B : public A {
public:
	int foo() override {
		return field + 20;
	}
};

class C {
public:
	int field_c;
	inline C() {
		field_c = 666;
	}
};

class D : public B, public C {
public:
	int foo() override {
		return field + 30;
	}
};


int main() {
	A* a = new A();
	A* b = new B();
	A* d = new D();
	a->field = 1;
	b->field = 2;

	B* b0 = dynamic_cast<B*>((A*) 0);
	B* b1 = dynamic_cast<B*>(a);
	B* b2 = dynamic_cast<B*>(b);
	B* b3 = dynamic_cast<B*>(d);
	C* c = dynamic_cast<C*>(d);


	printf("b0 = %p == %p\n", b0, (void*) 0);
	printf("b1 = %p == %p\n", b1, (void*) 0);
	printf("b2 = %p == %p\n", b2, b);
	printf("b3 = %p == %p\n", b3, d);
	printf("c  = %p == %p\n", c, 12+(char*)d);
	// cast them all to void*
	printf("a  = %p\n", dynamic_cast<void*>(a));
	printf("b  = %p\n", dynamic_cast<void*>(b));
	printf("d  = %p\n", dynamic_cast<void*>(d));
	printf("b0 = %p\n", dynamic_cast<void*>(b0));
	printf("b1 = %p\n", dynamic_cast<void*>(b1));
	printf("b2 = %p\n", dynamic_cast<void*>(b2));
	printf("b3 = %p\n", dynamic_cast<void*>(b3));
	printf("c  = %p\n", dynamic_cast<void*>(b3));

	return 0;
}
