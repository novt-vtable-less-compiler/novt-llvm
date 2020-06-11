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


template<int dim>
int Base<dim>::calc(std::vector<Storage<dim>>& v) {
	int result = 100 * dim;
	for (auto x: v) {
		result += x.data;
	}
	return result;
}

template<int dim>
Base<dim>::~Base() {
	printf(" [del base %d] ", dim);
}

template<int dim>
Child<dim>::~Child() {
	printf(" [del child %d] ", dim);
}


template<int dim>
int Child<dim>::calc(std::vector<Storage<dim>>& v) {
	int result = 1100 * dim;
	for (auto x: v) {
		result += x.data;
	}
	return result;
}


template class Base<1>;
template class Base<2>;
template class Base<3>;
template class Child<1>;
template class Child<2>;
template class Child<3>;
