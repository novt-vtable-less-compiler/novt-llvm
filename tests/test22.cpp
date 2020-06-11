#include <iostream>
#include <sstream>
#include <string>


void __attribute__ ((noinline)) fill_stream(std::ostream& stream) {
	stream << "Hello ";
	stream << "World";
	stream << std::endl;
}


class my_exception1 : public std::exception {
};


class my_exception2 : public std::exception {
public:
	virtual const char* what() const noexcept override {
		return "my_exception2 was here";
	}
};


void fun(int x) {
	switch (x) {
		case 0: throw std::exception();
		case 1: throw std::runtime_error("runtime_error with 1");
		case 2: throw my_exception1();
		case 3: throw my_exception2();
	}
}




int main() {
	fill_stream(std::cout);
	fill_stream(std::cerr);
	std::stringstream ss;
	fill_stream(ss);
	std::cout << ss.str();

	for (int i = 0; i < 5; i++) {
		try {
			fun(i);
			std::cout << i << ": no throw" << std::endl;
		} catch (std::exception& e) {
			std::cout << i << ": catch " << e.what() << std::endl;
		}
	}

	return 0;
}
