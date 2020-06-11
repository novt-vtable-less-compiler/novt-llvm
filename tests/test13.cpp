#include "test13.h"

int main() {
	{
		E e_stack;
		D* d = new D();
		d->field_d = 40;
		//*
		A* da = d;
		A* a = new A();
		A* b = new B();
		A* c = new C();
		A* e = new E();
		// */
		printf("d      @ %p\n", d);
		printf("d as A @ %p\n", (A*) d);
		printf("d as B @ %p\n", (B*) d);
		printf("d as C @ %p\n", (C*) d);
		printf("d->a   @ %p\n", &d->field_a);
		printf("d->b   @ %p\n", &d->field_b);
		printf("d->c   @ %p\n", &d->field_c);
		printf("d->d   @ %p\n", &d->field_d);

	//*
		printf("e->a   @ %p\n", &e_stack.field_a);
		printf("e->b   @ %p\n", &e_stack.field_b);
		printf("e->c   @ %p\n", &e_stack.field_c);
		printf("e->d   @ %p\n", &e_stack.field_d);
		printf("e->e   @ %p\n", &e_stack.field_e);

		printf("d->f  : %ld == 44\n", d->f());
		printf("da->f : %ld == 44\n", da->f());
		printf("a->f  : %ld == 11\n", a->f());
		printf("b->f  : %ld == 22\n", b->f());
		printf("c->f  : %ld == 33\n", c->f());
		printf("e->f  : %ld == 55\n", e->f());

		delete da;
		delete b;
		delete c;
		delete e;
	// */
	}

	printf("\n");

	return 0;
}
