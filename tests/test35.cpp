// num_classes=3, class_notempty=[True, True, True], inheritance_vec=[2, 2, 2]

#include <stdio.h>

class A {
public:
	char space[8];
	A() { printf("[new A @ %p] ", this); f(); printf("\n"); }
	virtual void t() { printf("  [T A @ %p]", this); }
	virtual void f() {
		printf("  [f A @ %p]", this);
		t();
	}
};

class B: public virtual A {
public:
	char space[24];
	B() { printf("[new B @ %p] ", this); f(); printf("\n"); }
	virtual void t() { printf("  [T B @ %p]", this); }
	virtual void f() {
		printf("  [f B @ %p]", this);
		A::f();
		t();
	}
};

class C: public virtual B, public virtual A {
public:
	char space[56];
	C() { printf("[new C @ %p] ", this); f(); printf("\n"); }
	virtual void t() { printf("  [T C @ %p]", this); }
	virtual void f() {
		printf("  [f C @ %p]", this);
		A::f();
		B::f();
		t();
	}
};



int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
	{
		A* a = new A();
		printf("A.f: ");
		a->f();
		printf("\n\n");
		delete a;
	}
	{
		B* b = new B();
		printf("B.f: ");
		b->f();
		printf("\n\n");
		delete b;
	}
	{
		C* c = new C();
		printf("C.f: ");
		c->f();
		printf("\n\n");
		delete c;
	}
	return 0;
}
