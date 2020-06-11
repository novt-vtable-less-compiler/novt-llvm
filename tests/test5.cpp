#include "test5.h"

int main() {
	A* a = new A();
	a->c = 10;
	A* b = new B();
	b->c = 20;
	A* c = new C();
	c->c = 30;

	Result r = a->f(7);
	printf("A: (%ld, %ld, %ld) == (10, 110, 17)\n", r.x, r.y, r.z);
	r = b->f(7);
	printf("B: (%ld, %ld, %ld) == (20, 220, 27)\n", r.x, r.y, r.z);
	r = c->f(7);
	printf("C: (%ld, %ld, %ld) == (30, 330, 37)\n", r.x, r.y, r.z);

	return 0;
}
