#include <fstream>
#include <iostream>

int main() {
	{
		std::ifstream f("/etc/passwd");
		std::string s;
		f >> s;
		std::cout << s << std::endl;
	}

	{
		std::ofstream f2("/tmp/xyz");
		f2 << "Hello World!\n";
	}

	{
		std::ifstream f("/tmp/xyz");
		std::string s;
		f >> s;
		std::cout << s << std::endl;
	}
}