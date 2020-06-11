#include <fstream>
#include <iostream>


class MemoryMapper {
public:
  virtual int allocateMappedMemory(int x) = 0;
};


namespace {
// Trivial implementation of SectionMemoryManager::MemoryMapper that just calls
// into sys::Memory.
class DefaultMMapper final : public MemoryMapper {
public:
  int allocateMappedMemory(int x) override {
  	printf("this = %p\n", this);
    return x+1;
  }
};

DefaultMMapper DefaultMMapperInstance;
}


int main() {
	printf("addr = %p\n", &DefaultMMapperInstance);
	printf("%d\n", DefaultMMapperInstance.allocateMappedMemory(2));
}

