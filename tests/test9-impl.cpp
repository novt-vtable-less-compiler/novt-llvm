#include "test9.h"

int B::testthrow(int x) {
	if (x == 1) {
		throw 1;
	} else if (x == 2) {
		throw "string 2";
	} else if (x == 3) {
		throw std::exception();
	} else if (x == 4) {
		throw MyException();
	} else if (x == 5) {
		throw MyBaseException();
	} else if (x == 6) {
		throw new MyException();
	} else if (x == 7) {
		throw new MyBaseException();
	}
	return x;
}


int external_stuff(int x) {
	if (x == 1) {
		throw 11;
	} else if (x == 2) {
		throw "B's string 2C";
	} else if (x == 3) {
		throw std::exception();
	} else if (x == 4) {
		throw MyException();
	} else if (x == 5) {
		throw MyBaseException();
	} else if (x == 6) {
		throw new MyException();
	} else if (x == 7) {
		throw new MyBaseException();
	}
	return x-1;
}


int C::testthrow(int x) {
	return external_stuff(x);
}

