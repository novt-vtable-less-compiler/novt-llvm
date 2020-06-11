#include <stdio.h>

/*
MEMORY LAYOUT:

  8 vtable        <---  +0
 16 S
 32 PS
  8 vtable DS     <--- +56
 16 S<DS
 64 DS
128 WS
  8 vtable Base   <-- +272
  8 Base

*/


class Base {
public:
	char x[8];
	virtual ~Base() = default;
	
	virtual void __attribute__ ((noinline)) f() {
		printf(" [Base::f %p]", this);
	}
};

class S : public virtual Base {
public:
	char x[16];
	virtual ~S() = default;

	void __attribute__ ((noinline)) f() override {
		printf(" [S::f %p]", this);
		Base::f();
	}
};

class PS : public S {
public:
	char x[32];
	virtual ~PS() = default;

	void __attribute__ ((noinline)) f() override {
		printf(" [PS::f %p]", this);
		S::f();
	}
};

class DS : public S {
public:
	char x[64];
	virtual ~DS() = default;

	void __attribute__ ((noinline)) f() override {
		printf(" [DS::f %p]", this);
		S::f();
	}
};

class WS : public PS, public DS {
public:
	char x[128];
	virtual ~WS() = default;

	void __attribute__ ((noinline)) f() override {
		printf(" [WS::f %p]", this);
		PS::f();
		printf(" |");
		DS::f();
		printf(" /");
	}
};




int main() {
	WS ws;
	printf("%p ws.f():", &ws);
	ws.f();
	printf("\n");

	Base* base = new WS();
	printf("%p base->f():", base);
	base->f();
	printf("\n");

	return 0;
}
