#include <stdio.h>


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
	d->field = 3;

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
	printf("b2->foo()  = %d  == 22\n", b2->foo());
	printf("b3->foo()  = %d  == 33\n", b3->foo());
	printf("c->field_c = %d == 666\n", c->field_c);

	return 0;
}
