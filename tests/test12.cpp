#include <stdio.h>
#include <stdlib.h>
#include <vector>


extern "C" {
	void* __attribute__ ((noinline)) _ZdlPvm(void* x) {
		printf(" [free %p]", x);
		free(x);
		return 0;
	}

	void* __attribute__ ((noinline)) _ZdlPv(void* x) {
		return _ZdlPvm(x);
	}

	void* __attribute__((noinline)) _Znwm(size_t size) {
		void* x = malloc(size);
		printf(" [malloc %ld %p]", size, x);
		return x;
	}
}




class A {
public:
	int index;
	char field[72];

	A(int index) : index(index) {
		printf(" [+A %d]", index);
	}

	virtual ~A(){
		printf(" [~A %d]", index);
	}
};

class B : public A {
public:
	B(int index) : A(index) {}

	virtual ~B(){
		printf(" [~B %d]", index);
	}	
};

class C : public B {
public:
	C(int index) : B(index) {}

	virtual ~C(){
		printf(" [~C %d]", index);
	}
};


int main() {
	A* a = new C(1);
	delete a;

	{
		A a2(2);
	}

	{
		std::vector<B> vec;
		vec.emplace_back(3);
		vec.emplace_back(4);
		vec.emplace_back(5);
		vec.emplace_back(6);
		vec.emplace_back(7);
	}

	printf("\n");

	return 0;
}
