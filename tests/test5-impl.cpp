#include "test5.h"

Result A::f(long a) {
	Result r;
	r.x = c;
	r.y = c + 100;
	r.z = c + a;
	return r;
}

Result B::f(long a) {
	Result r;
	r.x = c;
	r.y = c + 200;
	r.z = c + a;
	return r;
}

Result C::f(long a) {
	Result r;
	r.x = c;
	r.y = c + 300;
	r.z = c + a;
	return r;
}
