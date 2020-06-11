#include "test28.h"
#include <stdio.h>


A::~A() {
	printf("  [~A %p]\n", this);
}

void A::f(int x) {
	printf("A::f(%d) %p\n", x, this);
}



int main() {
	A* a = getA1();
	a->f(1);
	delete a;

	a = getA2();
	a->f(1);
	delete a;

	a = getB1();
	a->f(1);
	delete a;

	a = getB2();
	a->f(1);
	delete a;

	a = getB3();
	a->f(1);
	delete a;

	return 0;
}