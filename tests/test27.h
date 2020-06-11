#include <stdio.h>

class Z;
class Y;



class A {
public:
	virtual Y& foo(Z& z);
};

class B : public virtual A {
public:
	Y& foo(Z& z) override;
};

class C : public virtual B {};


A* get_some_class(int i);

void dispatch(A* a, Y& (A::*test)(Z&));
