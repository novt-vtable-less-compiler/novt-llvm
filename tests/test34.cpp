// num_classes=4, class_notempty=[True, True, True, True], inheritance_vec=[0, 2, 0, 0, 2, 1]

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

class B {
public:
	char space[24];
	virtual void t() { printf("  [T B @ %p]", this); }
	virtual void f() {
		printf("  [f B @ %p]", this);
		t();
	}
};

class C: public virtual A {
public:
	char space[56];
	virtual void t() { printf("  [T C @ %p]", this); }
	virtual void f() {
		printf("  [f C @ %p]", this);
		A::f();
		t();
	}
};

class D: public virtual B, public C {
public:
	char space[120];
	virtual void t() { printf("  [T D @ %p]", this); }
	virtual void f() {
		printf("  [f D @ %p]", this);
		B::f();
		C::f();
		t();
	}
};



int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
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
	{
		D* d = new D();
		printf("D.f: ");
		d->f();
		printf("\n");
		delete d;
	}
	return 0;
}
