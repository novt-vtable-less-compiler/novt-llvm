#include <stdio.h>
#include <typeinfo>

class A00 {
public:
	//virtual void f() { printf("  [A00::f]      "); }
	//virtual void f() const { printf("  [A00::f const]"); }
};

class A01 {
public:
	//virtual void f() { printf("  [A01::f]      "); }
	virtual void f() const { printf("  [A01::f const]"); }
};

class A10 {
public:
	virtual void f() { printf("  [A10::f]      "); }
	//virtual void f() const { printf("  [A10::f const]"); }
};

class A11 {
public:
	virtual void f() { printf("  [A11::f]      "); }
	virtual void f() const { printf("  [A11::f const]"); }
};





class B00_A00 : public A00 {
public:
	//void f() override { printf("  [B00::f]      "); }
	//void f() const override { printf("  [B00::f const]"); }
};

class B01_A00 : public A00 {
public:
	//void f() override { printf("  [B01::f]      "); }
	virtual void f() const { printf("  [B01::f const]"); }
};

class B10_A00 : public A00 {
public:
	virtual void f() { printf("  [B10::f]      "); }
	//void f() const override { printf("  [B10::f const]"); }
};

class B11_A00 : public A00 {
public:
	virtual void f() { printf("  [B11::f]      "); }
	virtual void f() const { printf("  [B11::f const]"); }
};




class B00_A01 : public A01 {
public:
	//void f() override { printf("  [B00::f]      "); A01::f(); }
	//void f() const override { printf("  [B00::f const]"); A01::f(); }
};

class B01_A01 : public A01 {
public:
	//void f() override { printf("  [B01::f]      "); A01::f(); }
	void f() const override { printf("  [B01::f const]"); A01::f(); }
};

class B10_A01 : public A01 {
public:
	virtual void f() { printf("  [B10::f]      "); A01::f(); }
	//void f() const override { printf("  [B10::f const]"); A01::f(); }
};

class B11_A01 : public A01 {
public:
	virtual void f() { printf("  [B11::f]      "); A01::f(); }
	void f() const override { printf("  [B11::f const]"); A01::f(); }
};




class B00_A10 : public A10 {
public:
	//void f() override { printf("  [B00::f]      "); A10::f(); }
	//void f() const override { printf("  [B00::f const]"); A10::f(); }
};

class B01_A10 : public A10 {
public:
	//void f() override { printf("  [B01::f]      "); A10::f(); }
	virtual void f() const { printf("  [B01::f const]"); }
};

class B10_A10 : public A10 {
public:
	void f() override { printf("  [B10::f]      "); A10::f(); }
	//void f() const override { printf("  [B10::f const]"); A10::f(); }
};

class B11_A10 : public A10 {
public:
	void f() override { printf("  [B11::f]      "); A10::f(); }
	virtual void f() const { printf("  [B11::f const]"); }
};




class B00_A11 : public A11 {
public:
	//void f() override { printf("  [B00::f]      "); A11::f(); }
	//void f() const override { printf("  [B00::f const]"); A11::f(); }
};

class B01_A11 : public A11 {
public:
	//void f() override { printf("  [B01::f]      "); A11::f(); }
	void f() const override { printf("  [B01::f const]"); A11::f(); }
};

class B10_A11 : public A11 {
public:
	void f() override { printf("  [B10::f]      "); A11::f(); }
	//void f() const override { printf("  [B10::f const]"); A11::f(); }
};

class B11_A11 : public A11 {
public:
	void f() override { printf("  [B11::f]      "); A11::f(); }
	void f() const override { printf("  [B11::f const]"); A11::f(); }
};



template<class T, class B>
inline void test() {
	T* t = new T();
	const T* tc = t;
	B* b = t;
	const B* bc = tc;
	printf("[child noconst] %s->f(): ", typeid(T).name());
	t->f();
	printf("\n");
	printf("[child const]   %s->f(): ", typeid(T).name());
	tc->f();
	printf("\n");
	printf("[base noconst]  %s->f(): ", typeid(T).name());
	b->f();
	printf("\n");
	printf("[base const]    %s->f(): ", typeid(T).name());
	bc->f();
	printf("\n");
}

template<class T, class B>
inline void test_nobase() {
	T* t = new T();
	const T* tc = t;
	B* b = t;
	const B* bc = tc;
	printf("[child noconst] %s->f(): ", typeid(T).name());
	t->f();
	printf("\n");
	printf("[child const]   %s->f(): ", typeid(T).name());
	tc->f();
	printf("\n");
	/*printf("[base noconst]  %s->f(): ", typeid(T).name());
	b->f();
	printf("\n");
	printf("[base const]    %s->f(): ", typeid(T).name());
	bc->f();
	printf("\n");*/
}

template<class T, class B>
inline void test_noconstbase() {
	T* t = new T();
	const T* tc = t;
	B* b = t;
	const B* bc = tc;
	printf("[child noconst] %s->f(): ", typeid(T).name());
	t->f();
	printf("\n");
	printf("[child const]   %s->f(): ", typeid(T).name());
	tc->f();
	printf("\n");
	printf("[base noconst]  %s->f(): ", typeid(T).name());
	b->f();
	printf("\n");
	/*printf("[base const]    %s->f(): ", typeid(T).name());
	bc->f();
	printf("\n");*/
}

template<class T, class B>
inline void test_noconst() {
	T* t = new T();
	const T* tc = t;
	B* b = t;
	const B* bc = tc;
	printf("[child noconst] %s->f(): ", typeid(T).name());
	t->f();
	printf("\n");
	/*printf("[child const]   %s->f(): ", typeid(T).name());
	tc->f();
	printf("\n");*/
	printf("[base noconst]  %s->f(): ", typeid(T).name());
	b->f();
	printf("\n");
	/*printf("[base const]    %s->f(): ", typeid(T).name());
	bc->f();
	printf("\n");*/
}

template<class T, class B>
inline void test_noconst_nobase() {
	T* t = new T();
	const T* tc = t;
	B* b = t;
	const B* bc = tc;
	printf("[child noconst] %s->f(): ", typeid(T).name());
	t->f();
	printf("\n");
	/*printf("[child const]   %s->f(): ", typeid(T).name());
	tc->f();
	printf("\n");
	printf("[base noconst]  %s->f(): ", typeid(T).name());
	b->f();
	printf("\n");
	printf("[base const]    %s->f(): ", typeid(T).name());
	bc->f();
	printf("\n");*/
}


int main() {
	// test<B00_A00>();
	test_nobase<B01_A00, A00>();
	test_noconst_nobase<B10_A00, A00>();
	test_nobase<B11_A00, A00>();
	test<B00_A01, A01>();
	test<B01_A01, A01>();
	test_noconst<B10_A01, A01>();
	test<B11_A01, A01>();
	test_noconst<B00_A10, A10>();
	test_noconstbase<B01_A10, A10>();
	test_noconst<B10_A10, A10>();
	test_noconstbase<B11_A10, A10>();
	test<B00_A11, A11>();
	test<B01_A11, A11>();
	test_noconst<B10_A11, A11>();
	test<B11_A11, A11>();
	return 0;
}


