#include "test6.h"

int A::testthrow(int x) {
	if (x == 1) {
		throw 1;
	} else if (x == 2) {
		throw "string 2";
	} else if (x == 3) {
		throw std::exception();
	}
	return x;
}


int external_stuff(int x) {
	if (x == 1) {
		throw 11;
	} else if (x == 2) {
		throw "B's string 2B";
	} else if (x == 3) {
		throw std::exception();
	}
	return x-1;
}


int B::testthrow(int x) {
	return external_stuff(x);
}

