#include <stdio.h>
#include <exception>

class A {
public:
	virtual int testthrow(int x) = 0;
};

class B : public A {
public:
	int testthrow(int x) override;
};

class C : public A {
public:
	int testthrow(int x) override;
};


class MyAbstractException {
public:
	virtual const char* what() const = 0;
	virtual ~MyAbstractException() = default;
};

class MyBaseException : public MyAbstractException {
public:
	virtual const char* what() const {
		return "MyBaseException";
	}

	virtual ~MyBaseException() {
		printf(" [~MyBaseException] ");
	}
};

class MyException : public MyBaseException {
public:
	const char* what() const override {
		return "MyException";
	}

	virtual ~MyException() {
		printf(" [~MyException] ");
	}
};
