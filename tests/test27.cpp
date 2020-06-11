#include "test27.h"



int main() {
	A* a = get_some_class(0);
	dispatch(a, &A::foo);

	a = get_some_class(1);
	dispatch(a, &A::foo);

	a = get_some_class(2);
	dispatch(a, &A::foo);

	a = get_some_class(3);
	dispatch(a, &A::foo);

	return 0;
}

