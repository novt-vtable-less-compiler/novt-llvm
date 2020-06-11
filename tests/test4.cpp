#include "test4.h"


int main(int argc, const char* argv[]) {
	puts("Welcome!");
	C *c = new C();
	A *a = (A*) c;
	D *d = new D();

#ifndef BUGTRACK
	printf("get_a            : %ld == 10\n", c->get_a());
	printf("get_b            : %ld == 20\n", c->get_b());
	printf("get_c            : %ld == 30\n", c->get_c());
	printf("get_some         : %ld == 20\n", c->get_some());
#endif
	printf("A.get_some       : %ld == 20\n", a->get_some());
#ifndef BUGTRACK
	printf("trigger_get_some : %ld == 21\n", a->trigger_get_some());
#endif
	printf("trigger_get_b2   : %ld == 33\n", c->trigger_get_b2());
	printf("trigger_get_b2 D : %ld == 44\n", d->trigger_get_b2());

	delete a;
	delete d;

	puts("Done.");
}
