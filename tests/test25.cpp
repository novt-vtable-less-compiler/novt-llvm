#include <stdio.h>


class A {
public:
	virtual void __attribute ((__abi_tag__ ("cxx11"))) message(int i);
};

class B : public A {
public:
	void message(int i) override;
};


void A::message(int i) {
	printf("A = %d\n", i);
}

void B::message(int i) {
	printf("B = %d\n", i);
}



int main() {
	A* a = new A();
	A* b = new B();
	a->message(15);
	b->message(17);
}
