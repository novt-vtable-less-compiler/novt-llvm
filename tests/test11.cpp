#include <stdio.h>


class A {
public:
	int field;

	virtual int foo(int *x) {
		return field + 10 + *x;
	}

	virtual long foo(long *x) {
		return field + 100 + *x;
	}
};

class B : public A {
public:
	int foo(int *x) override {
		return field + 20 + *x;
	}

	long foo(long *x) override {
		return field + 200 + *x;
	}
};

class C : public B {
public:
	int foo(int *x) override {
		return field + 30 + *x;
	}

	long foo(long *x) override {
		return field + 300 + *x;
	}
};


int main() {
	A* a = new A();
	A* b = new B();
	A* c = new C();
	a->field = 4;
	b->field = 5;
	c->field = 6;

	int i = 1000;
	long l = 1000;

	printf("A int 1000:  %d == 1014\n", a->foo(&i));
	printf("A long 1000: %ld == 1104\n", a->foo(&l));
	printf("B int 1000:  %d == 1025\n", b->foo(&i));
	printf("B long 1000: %ld == 1205\n", b->foo(&l));
	printf("C int 1000:  %d == 1036\n", c->foo(&i));
	printf("C long 1000: %ld == 1306\n", c->foo(&l));

	return 0;
}
