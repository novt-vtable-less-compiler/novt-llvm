#include <stdio.h>

class A {
public:
	virtual long f();
	virtual long h();
};

class B : public virtual A {
public:
	virtual long g() = 0;
};

class C : public B {
public:
	long f() override;
	long g() override;
};

long A::f() {
	return 1;
}
long A::h() {
	return 100;
}
long C::f() {
	return 3;
}
long C::g() {
	return 30;
}


int main() {
	A* a = new A();
	A* c = new C();
	C* c2 = new C();

	printf("A->f :   %ld ==   1\n", a->f());
	printf("A->h : %ld == 100\n", a->h());
	printf("C->f :   %ld ==   3\n", c->f());
	printf("C->h : %ld == 100\n", c->h());
	printf("C->g :  %ld ==  30\n", c2->g());

	return 0;
}
