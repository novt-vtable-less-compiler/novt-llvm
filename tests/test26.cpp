#include <stdio.h>
#include <string>


#define FileOperationResult int
#define FileOffset long


class FileSeekerInterface {
 public:
  virtual FileOffset Seek(FileOffset offset, int whence) = 0;

  // FileOffset SeekGet();

  // bool SeekSet(FileOffset offset);

 protected:
  ~FileSeekerInterface() {
  	printf("  [~FileSeekerInterface %p]", this);
  }
};

class FileReaderInterface : public virtual FileSeekerInterface {
 public:
  virtual ~FileReaderInterface() {
    printf("  [~FileReaderInterface %p]", this);
  }

  virtual FileOperationResult Read(void* data, size_t size) = 0;

  // bool ReadExactly(void* data, size_t size);
};


class FileWriterInterface : public virtual FileSeekerInterface {
 public:
  virtual ~FileWriterInterface() {
  	printf("  [~FileWriterInterface %p]", this);
  }

  virtual bool Write(const void* data, size_t size) = 0;

  // virtual bool WriteIoVec(std::vector<WritableIoVec>* iovecs) = 0;
};


class StringFile : public FileReaderInterface, public FileWriterInterface {
 public:
  StringFile();
  ~StringFile() override;

  const std::string& string() const { return string_; }

  // void SetString(const std::string& string);

  // void Reset();

  // FileReaderInterface:
  FileOperationResult Read(void* data, size_t size) override;

  // FileWriterInterface:
  bool Write(const void* data, size_t size) override;
  // bool WriteIoVec(std::vector<WritableIoVec>* iovecs) override;

  // FileSeekerInterface:
  FileOffset Seek(FileOffset offset, int whence) override;

 private:
  std::string string_;

  // base::CheckedNumeric<size_t> offset_;
  long offset_;
};





FileOperationResult StringFile::Read(void* data, size_t size) {
	printf("[StringFile::%s @ %p]\n", __func__, this);
	return 0;
}

bool StringFile::Write(const void* data, size_t size) {
	printf("[StringFile::%s @ %p]\n", __func__, this);
	return true;
}

FileOffset StringFile::Seek(FileOffset offset, int whence) {
	printf("[StringFile::%s @ %p]\n", __func__, this);
	return 0;
}

StringFile::StringFile() {
	printf("New Stringfile @ %p\n", this);
}

StringFile::~StringFile() {
	printf("  [~StringFile %p]", this);
}






// Something to compare with
struct A {
	int i;
	virtual void f() = 0;
};
struct B : public virtual A {
	virtual void g() = 0;
};
struct C : public virtual A {
	virtual void h() = 0;
};
struct D : public B, public C {
	int i2;
	void f() override {
		printf("[D::%s @ %p]\n", __func__, this);
	}
	void g() override {
		printf("[D::%s @ %p]\n", __func__, this);
	}
	void h() override {
		printf("[D::%s @ %p]\n", __func__, this);
	}
};






int main() {
	StringFile* sf = new StringFile();
	FileReaderInterface* ri = sf;
	FileWriterInterface* wi = sf;
	FileSeekerInterface* si = sf;
	FileSeekerInterface* si2 = ri;
	FileSeekerInterface* si3 = wi;
	sf->Read(nullptr, 0);
	sf->Write(nullptr, 0);
	sf->Seek(15, 0);

	printf("-----\n");
	ri->Read(nullptr, 1);
	ri->Seek(15, 1);

	wi->Write(nullptr, 2);
	wi->Seek(15, 2);

	si->Seek(18, 11);
	si2->Seek(18, 12);
	si3->Seek(18, 13);

	printf("Delete: ");
	delete wi;
	printf("\n");

	D* d = new D();
	A* a = d;
	B* b = d;
	C* c = d;
	A* a2 = b;
	A* a3 = c;
	d->f();
	d->g();
	d->h();
	a->f();
	b->f();
	b->g();
	c->f();
	c->h();
	a2->f();
	a3->f();
}
