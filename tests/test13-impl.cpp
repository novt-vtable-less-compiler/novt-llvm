#include "test13.h"



A::~A() {
	printf(" [~A]");
}
B::~B() {
	printf(" [~B]");
}
C::~C() {
	printf(" [~C]");
}
D::~D() {
	printf(" [~D]");
}
E::~E() {
	printf(" [~E]");
}



long A::f() {
	return field_a + 1;
}
long B::f() {
	return field_b + 2;
}
long C::f() {
	return field_c + 3;
}
long D::f() {
	return field_d + 4;
}
long E::f() {
	return field_e + 5;
}
