#include <stdio.h>


class A {
public:
	long field_a;
	inline A() : field_a(10){}
	virtual ~A();
	virtual long f();
};

class B : public virtual A {
public:
	long field_b;
	inline B() : field_b(20){}
	virtual ~B();
	long f() override;
};

class C : public virtual A {
public:
	long field_c;
	inline C() : field_c(30){}
	virtual ~C();
	long f() override;
};

class D : public B, public C {
public:
	long field_d;
	inline D() : field_d(40){}
	virtual ~D();
	long f() override;
};

class E : public D {
public:
	long field_e;
	inline E() : field_e(50){}
	virtual ~E();
	long f() override;
};

