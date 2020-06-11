#include <stdio.h>


class Base {
public:
	int field_base = 0x11111111;
	virtual ~Base() = default;
	virtual void setOnBase(int value) {
		printf("[%s] Write to %p ...\n", __func__, &field_base);
		field_base = value;
	}

	virtual void debug() {
		printf("Debug Base %p: 0x%x\n", this, field_base);
	};
};

class S : public virtual Base {
public:
	virtual ~S() = default;
	long field_S = 0x4444444444444444;
};

class PS : public S {
public:
	virtual ~PS() = default;
	long shit;
	long field_PS = 0x5555555555555555;
};

class DS : public S {
public:
	virtual ~DS() = default;
	long field_DS = 0x5555555555555555;
};

class WS : public PS, public DS {
public:
	virtual ~WS() = default;
	long field_WS = 0x6666666666666666;

	void debug() override {
		char* ws = (char*) this;
		char* ps = (char*) (PS*) this;
		char* ds = (char*) (DS*) this;
		char* base = (char*) (Base*) this;
		printf("Debug WS: %p (PS %p   DS %p   Base %p)\n", this, (PS*) this, (DS*) this, (Base*) this);
		printf("          (offsets %ld %ld %ld)\n", ps - ws, ds - ws, base - ws);
		printf("      PS: 0x%lx  0x%lx\n", this->field_PS, ((PS*) this)->field_S);
		printf("      DS: 0x%lx  0x%lx\n", this->field_DS, ((DS*) this)->field_S);
		printf("      Base: 0x%x\n", this->field_base);
	}
};




int main() {
	Base* base = new WS();
	printf("base = %p\n", base);
	base->debug();
	base->setOnBase(0x22222222);
	base->debug();

	return 0;
}
