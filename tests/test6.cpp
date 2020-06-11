#include "test6.h"

void test(A* a, int x) {
	try {
		int y = a->testthrow(x);
		printf("returned %d", y);
	} catch (int z) {
		printf("throw int %d", z);
	} catch (const char* c) {
		printf("throw char* \"%s\"", c);
	} catch (const std::exception& e) {
		printf("exception \"%s\"", e.what());
	} catch (...) {
		printf("throw unknown");
	}
}


int main() {
	A* a = new A();
	A* b = new B();

	printf("A 0 : ");
	test(a, 0);
	puts(" == returned 0");
	printf("B 0 : ");
	test(b, 0);
	puts(" == returned -1");

	printf("A 1 : ");
	test(a, 1);
	puts(" == throw int 1");
	printf("B 1 : ");
	test(b, 1);
	puts(" == throw int 11");

	printf("A 2 : ");
	test(a, 2);
	puts(" == throw char* \"string 2\"");
	printf("B 2 : ");
	test(b, 2);
	puts(" == throw char* \"B's string 2B\"");

	printf("A 3 : ");
	test(a, 3);
	puts(" == exception \"std::exception\"");
	printf("B 3 : ");
	test(b, 3);
	puts(" == exception \"std::exception\"");

	return 0;
}