#include <stdio.h>

#define BUGTRACK

class A {
public:
	long field_a;

	virtual long get_some();
#ifndef BUGTRACK
	virtual long get_a();
	long trigger_get_some();
#endif
	virtual ~A();
};

class B {
public:
	long field_b;
	virtual long get_b();
	virtual long get_b2();
	long trigger_get_b2();

	virtual ~B();
};

class C : public A, public B {
public:
	long field_c;
	C();
#ifndef BUGTRACK
	virtual long get_c();
	long get_some() override;
#endif
	long get_b2() override;

	virtual ~C();
};

class A2 {
public:
	long field_a2;
	short field_a3;
	virtual long a2() = 0;

	virtual ~A2();
};

class D : public A2, public C {
public:
	long field_d = 40;
	long get_b2() override;
	long a2() override;

	virtual ~D();
};
