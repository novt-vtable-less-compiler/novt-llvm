#include <stdio.h>

class A {
public:
	int a = 10;
};

int main() {
	A* a = new A();
	printf("Hello World!\n");
	printf("%d\n", a->a);
	return 0;
}
