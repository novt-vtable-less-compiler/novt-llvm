#include <stdio.h>

class Result {
public:
	long x;
	long y;
	long z;
};

class A {
public:
	long c;
	virtual Result f(long a);
};

class B : public A {
public:
	Result f(long a) override;
};

class C : public A {
public:
	Result f(long a) override;
};
