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


class WS;


class Base {
public:
	char x[8];
	virtual ~Base() = default;

	Base() {
		printf("Create Base : %p", this);
		this->f();
		printf("\n");
	}
	
	virtual void __attribute__ ((noinline)) f() {
		printf(" [Base::f %p]", this);
	}
};

class S : public virtual Base {
public:
	char x[16];
	virtual ~S() = default;

	S() {
		printf("Create S : %p", this);
		this->f();
		printf("\n");
	}

	void __attribute__ ((noinline)) f() override {
		printf(" [S::f %p]", this);
		Base::f();
		g();
	}

	// virtual void __attribute__ ((noinline)) g() = 0;
	virtual void __attribute__ ((noinline)) g() {
		printf(" [S::g %p]", this);
	}
};

class PS : public S {
public:
	char x[32];
	virtual ~PS() = default;

	PS() {
		printf("Create PS : %p", this);
		this->f();
		printf("\n");
	}

	void __attribute__ ((noinline)) f() override {
		printf(" [PS::f %p]", this);
		S::f();
	}

	void __attribute__ ((noinline)) g() override {
		printf(" [PS::g %p]", this);
		//S::g();
	}
};

class DS : public S {
public:
	char x[64];
	virtual ~DS() = default;

	DS();

	void __attribute__ ((noinline)) f() override {
		printf(" [DS::f %p]", this);
		S::f();
	}

	void __attribute__ ((noinline)) g() override {
		printf(" [DS::g %p]", this);
		//S::g();
	}
};

class WS : public PS, public DS {
public:
	char x[128];
	virtual ~WS() = default;

	WS() {
		printf("Create WS : %p", this);
		this->f();
		printf("\n");
	}

	void __attribute__ ((noinline)) f() override {
		printf(" [WS::f %p]", this);
		PS::f();
		printf(" |");
		DS::f();
		printf(" /");
	}
};

class WS2 : public WS {
public:
	char y;

	WS2() {
		printf("Create WS2 : %p", this);
		this->f();
		printf("\n");
	}

	void __attribute__ ((noinline)) f() override {
		printf(" [WS2::f %p]", this);
		WS::f();
	}
};


DS::DS() {
	printf("Create DS : %p", this);
	this->f();
	printf("   cast to WS: %p   cast to void*: %p\n", dynamic_cast<WS*>(this), dynamic_cast<void*>(this));
}



int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
	WS ws;
	printf("\n%p ws.f():", &ws);
	ws.f();
	printf("\n");

	Base* base = new WS();
	printf("\n%p base->f():", base);
	base->f();
	printf("\n");

	base = new WS2();
	printf("\n%p base2->f():", base);
	base->f();
	printf("\n");

	return 0;
}
