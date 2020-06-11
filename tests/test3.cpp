#include "sample_classes_def.h"


int main(int argc, const char* argv[]) {
	puts("Welcome!");
	A *a = get(argv[1]);

	const auto &t1 = typeid(*a);
	//std::cout << "Type = " << t1.name() << "\n";
	printf("Type: %s\n", t1.name());

	puts("Done.");
}
