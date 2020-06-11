#include <stdio.h>
#include <typeinfo>


class Handler {
public:
	virtual int dosth(int x) = 0;
};

template<class T>
class TypeHandler : public Handler {
public:
	int dosth(int x) {
		const auto &t = typeid(T);
		printf("%s", t.name());
		return x;
	}
};


int main() {
	Handler* h = new TypeHandler<int>();
	Handler* h2 = new TypeHandler<const char*>();
	Handler* h3 = new TypeHandler<Handler>();
	printf(" == int, %d == 0\n", h->dosth(0));
	printf(" == const char*, %d == 0\n", h2->dosth(0));
	printf(" == Handler, %d == 0\n", h3->dosth(0));
	return 0;
}
