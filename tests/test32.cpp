#include <stdio.h>

class A {
public:
	char space[8];
	virtual void t() { printf("  [T A @ %p]", this); }
	virtual void f() {
		printf("  [f A @ %p]", this);
		t();
	}
};

class B: public virtual A {
public:
	char space[24];
	virtual void t() { printf("  [T B @ %p]", this); }
	virtual void f() {
		printf("  [f B @ %p]", this);
		A::f();
		t();
	}
};

class C: public virtual B {
public:
	char space[56];
	virtual void t() { printf("  [T C @ %p]", this); }
	virtual void f() {
		printf("  [f C @ %p]", this);
		B::f();
		t();
	}
};



int main() {
	{
		A* a = new A();
		printf("A.f: ");
		a->f();
		printf("\n");
		delete a;
	}
	{
		B* b = new B();
		printf("B.f: ");
		b->f();
		printf("\n");
		delete b;
	}
	{
		C* c = new C();
		printf("C.f: ");
		c->f();
		printf("\n");
		delete c;
	}
	return 0;
}
