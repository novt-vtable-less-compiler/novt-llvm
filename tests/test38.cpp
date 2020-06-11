// num_classes=7, class_notempty=[False, True, True, True, False, True, True], inheritance_vec=[2, 2, 2, 2, 2, 1, 2, 2, 1, 0, 0, 0, 2, 1, 2, 2, 1, 2, 0, 2, 1]

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
	char space[8];
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

class C: public virtual A, public virtual B {
public:
	char space[24];
	C() {
		printf("[new C @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T C @ %p]", this); }
	virtual void f() {
		printf("  [f C @ %p]", this);
		A::f();
		B::f();
		t();
	}
};

class D: public virtual A, public virtual B, public C {
public:
	char space[56];
	D() {
		printf("[new D @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T D @ %p]", this); }
	virtual void f() {
		printf("  [f D @ %p]", this);
		A::f();
		B::f();
		C::f();
		t();
	}
};

class E: public virtual B, public virtual A, public C {
public:
	E() {
		printf("[new E @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T E @ %p]", this); }
	virtual void f() {
		printf("  [f E @ %p]", this);
		A::f();
		B::f();
		C::f();
		t();
	}
};

class F: public virtual C, public virtual E, public D {
public:
	char space[120];
	F() {
		printf("[new F @ %p] ", this);
		f();
		printf("\n");
	}
	virtual void t() { printf("  [T F @ %p]", this); }
	virtual void f() {
		printf("  [f F @ %p]", this);
		// C::f();
		D::f();
		E::f();
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
		A* c0 = (A*) c;
		c0->f();
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
		A* d0 = (A*) d;
		d0->f();
		printf("\n");
		B* d1 = (B*) d;
		d1->f();
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
		A* e0 = (A*) e;
		e0->f();
		printf("\n");
		B* e1 = (B*) e;
		e1->f();
		printf("\n");
		C* e2 = (C*) e;
		e2->f();
		printf("\n");
		delete e;
		printf("\n");
	}
	{
		F* f = new F();
		printf("F.f: ");
		f->f();
		printf("\n");
		A* f0 = (A*) f;
		f0->f();
		printf("\n");
		B* f1 = (B*) f;
		f1->f();
		printf("\n");
		D* f3 = (D*) f;
		f3->f();
		printf("\n");
		E* f4 = (E*) f;
		f4->f();
		printf("\n");
		delete f;
		printf("\n");
	}
	return 0;
}
