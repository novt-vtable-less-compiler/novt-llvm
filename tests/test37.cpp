// num_classes=5, class_notempty=[False, False, True, True, False], inheritance_vec=[2, 1, 2, 1, 0, 1, 0, 1, 2, 2]

#include <stdio.h>

class A {
public:
	A() {
		printf("[new A @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T A @ %p]", this); }
	virtual void f() {
		printf("  [f A @ %p]", this);
		t();
	}
};

class B: public virtual A {
public:
	B() {
		printf("[new B @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T B @ %p]", this); }
	virtual void f() {
		printf("  [f B @ %p]", this);
		A::f();
		t();
	}
};

class C: public virtual B, public A {
public:
	char space[8];
	C() {
		printf("[new C @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T C @ %p]", this); }
	virtual void f() {
		printf("  [f C @ %p]", this);
		// A::f();
		B::f();
		t();
	}
};

class D: public C, public A {
public:
	char space[24];
	D() {
		printf("[new D @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T D @ %p]", this); }
	virtual void f() {
		printf("  [f D @ %p]", this);
		// A::f();
		C::f();
		t();
	}
};

class E: public virtual C, public virtual D, public B {
public:
	E() {
		printf("[new E @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T E @ %p]", this); }
	virtual void f() {
		printf("  [f E @ %p]", this);
		// B::f();
		// C::f();
		D::f();
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
		printf("\n");
	}
	{
		B* b = new B();
		printf("B.f: ");
		b->f();
		printf("\n");
		A* b0 = (A*) b;
		b0->f();
		printf("\n");
		delete b;
		printf("\n");
	}
	{
		C* c = new C();
		printf("C.f: ");
		c->f();
		printf("\n");
		B* c1 = (B*) c;
		c1->f();
		printf("\n");
		delete c;
		printf("\n");
	}
	{
		D* d = new D();
		printf("D.f: ");
		d->f();
		printf("\n");
		C* d2 = (C*) d;
		d2->f();
		printf("\n");
		delete d;
		printf("\n");
	}
	{
		E* e = new E();
		printf("E.f: ");
		e->f();
		printf("\n");
		D* e3 = (D*) e;
		e3->f();
		printf("\n");
		delete e;
		printf("\n");
	}
	return 0;
}
