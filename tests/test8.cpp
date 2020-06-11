#include <stdio.h>
#include <vector>


template<int dim>
class Storage {
public:
	int data;
	inline Storage(int data) : data(data){}
};




template<int dim>
class Base {
public:
	virtual int calc(std::vector<Storage<dim>>& v);
	virtual ~Base();
};


template<int dim>
class Child : public Base<dim> {
public:
	virtual int calc(std::vector<Storage<dim>>& v);
	virtual ~Child();
};



int main() {
	Base<1>& b1 = *(new Base<1>());
	Base<1>& c1 = *(new Child<1>());
	Base<2>& b2 = *(new Base<2>());
	Base<2>& c2 = *(new Child<2>());
	Base<3>& b3 = *(new Base<3>());
	Base<3>& c3 = *(new Child<3>());

	std::vector<Storage<1>> data1;
	std::vector<Storage<2>> data2;
	std::vector<Storage<3>> data3;
	data1.emplace_back(5);
	data1.emplace_back(11);
	data1.emplace_back(3);


	printf("b1 : %d == 119\n", b1.calc(data1));
	printf("c1 : %d == 1119\n", c1.calc(data1));
	printf("b2 : %d == 200\n", b2.calc(data2));
	printf("c2 : %d == 2200\n", c2.calc(data2));
	printf("b3 : %d == 300\n", b3.calc(data3));
	printf("c3 : %d == 3300\n", c3.calc(data3));

	delete &b1;
	delete &c1;
	delete &b2;
	delete &c2;
	delete &b3;
	delete &c3;

	printf("\n");

	return 0;
}
