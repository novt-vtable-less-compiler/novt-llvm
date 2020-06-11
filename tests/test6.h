#include <stdio.h>
#include <exception>

class A {
public:
	virtual int testthrow(int x);
};

class B : public A {
public:
	int testthrow(int x) override;
};
