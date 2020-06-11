#include "test4.h"

long A::get_some() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_a;
}
#ifndef BUGTRACK
long A::get_a() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_a;
}
long A::trigger_get_some() {
	printf("this = %p  (%s)\n", this, __func__);
	return this->get_some() + 1;
}
#endif
long B::get_b() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_b;
}
long B::get_b2() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_b+2;
}
long B::trigger_get_b2() {
	printf("this = %p  (%s)\n", this, __func__);
	return this->get_b2() + 1;
}
#ifndef BUGTRACK
long C::get_c() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_c;
}
long C::get_some() {
	printf("this = %p  (%s)\n", this, __func__);
	return B::get_b();
}
#endif
long C::get_b2() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_c+2;
}

C::C() : field_c(30){
	field_a = 10;
	field_b = 20;
}


long D::get_b2() {
	printf("this = %p  (%s)\n", this, __func__);
	return field_d+3;
}

long D::a2() {
	return 15;
}

A::~A() { puts("~A"); }
B::~B() { puts("~B"); }
C::~C() { puts("~C"); }
D::~D() { puts("~D"); }
A2::~A2() { puts("~A2"); }