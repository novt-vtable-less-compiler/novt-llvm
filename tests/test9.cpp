#include "test9.h"

void test(A* a, int x) {
	try {
		int y = a->testthrow(x);
		printf("returned %d", y);
	} catch (int z) {
		printf("throw int %d", z);
	} catch (const char* c) {
		printf("throw char* \"%s\"", c);
	} catch (const MyException& e) {
		printf("my_exception \"%s\"", e.what());
	} catch (const MyBaseException& e) {
		printf("my_base_exception \"%s\"", e.what());
	} catch (const std::exception& e) {
		printf("exception \"%s\"", e.what());
	} catch (MyException* e) {
		printf("my_exception_ptr \"%s\"", e->what());
	} catch (MyBaseException* e) {
		printf("my_base_exception_ptr \"%s\"", e->what());
	} catch (...) {
		printf("throw unknown");
	}
}


int main() {
	A* b = new B();
	A* c = new C();

	printf("B 0 : ");
	test(b, 0);
	puts(" == returned 0");
	printf("C 0 : ");
	test(c, 0);
	puts(" == returned -1");

	printf("B 1 : ");
	test(b, 1);
	puts(" == throw int 1");
	printf("C 1 : ");
	test(c, 1);
	puts(" == throw int 11");

	printf("B 2 : ");
	test(b, 2);
	puts(" == throw char* \"string 2\"");
	printf("C 2 : ");
	test(c, 2);
	puts(" == throw char* \"B's string 2B\"");

	printf("B 3 : ");
	test(b, 3);
	puts(" == exception \"std::exception\"");
	printf("C 3 : ");
	test(c, 3);
	puts(" == exception \"std::exception\"");

	printf("B 4 : ");
	test(b, 4);
	puts(" == exception \"MyException\"");
	printf("C 4 : ");
	test(c, 4);
	puts(" == exception \"MyException\"");

	printf("B 5 : ");
	test(b, 5);
	puts(" == exception \"MyBaseException\"");
	printf("C 5 : ");
	test(c, 5);
	puts(" == exception \"MyBaseException\"");

	printf("B 6 : ");
	test(b, 6);
	puts(" == exception \"MyException ptr\"");
	printf("C 6 : ");
	test(c, 6);
	puts(" == exception \"MyException ptr\"");

	printf("B 7 : ");
	test(b, 7);
	puts(" == exception \"MyBaseException ptr\"");
	printf("C 7 : ");
	test(c, 7);
	puts(" == exception \"MyBaseException ptr\"");

	return 0;
}