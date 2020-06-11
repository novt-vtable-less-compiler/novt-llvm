#define CORE_EXPORT 
#define PLATFORM_EXPORT 
#define DEFINE_WRAPPERTYPEINFO int wrapper

#include "test31_garbage.h"
#include <stdio.h>


extern "C" {
  void __attribute__((noinline)) break_here();

  void break_here(){
    puts("hi");
  }
}



namespace blink {

struct ContainerNode { int x = 0xffff; virtual void f(){} };
struct ConsoleLogger {
  int x = 0xffff;
  virtual void f(){}
  virtual void cl() {
    printf("[ConsoleLogger = %p]\n", this);
  }
};
struct TreeScope { int x = 0xfffe; virtual void g(){} };
struct UseCounter {
	virtual TraceDescriptor GetTraceDescriptor() const {
		return {};
	}
	virtual HeapObjectHeader* GetHeapObjectHeader() const {
		return nullptr;
	}
	int x = 0xffff;
  virtual void uc() {
    printf("[UseCounter = %p]\n", this);
  }
  virtual void uc2() {}

  static void test(UseCounter* uc) {
    printf("as usecounter: %p\n", uc);
    uc->uc();
    uc->uc2();
  }
};
struct FeaturePolicyParserDelegate { int x = 0xffff; };
struct ContextLifecycleNotifier {
	int x = 0xffff;
	virtual ~ContextLifecycleNotifier(){}
};

struct HeapObjectHeader {
	template<class T>
	static HeapObjectHeader* FromPayload(const T* t) { return nullptr; }
};



class CORE_EXPORT ExecutionContext
    : //public Supplementable<ExecutionContext>,
      public ContextLifecycleNotifier,
      public virtual ConsoleLogger,
      public virtual UseCounter,
      public virtual FeaturePolicyParserDelegate {
  MERGE_GARBAGE_COLLECTED_MIXINS();

 public:
  virtual bool IsContextThread() const {
  	printf("[ExecutionContext::IsContextThread %p]\n", this);
  	return true;
  }

  void debugExecutionContext() {
    printf("[ExecutionContext::debugExecutionContext %p]\n", this);
    cl();
    uc();
  }
 protected:
  explicit ExecutionContext(int* isolate);
  ~ExecutionContext() override;
 long some_fields;
};


class CORE_EXPORT Document : public ContainerNode,
                             public TreeScope,
                             public virtual ConsoleLogger,
                             public virtual UseCounter,
                             public virtual FeaturePolicyParserDelegate,
                             private ExecutionContext
                             //, public Supplementable<Document>
                              {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(Document);

 public:
  static Document* Create(Document&);
  Document();
  explicit Document(const long& init);
  ~Document() override;

  bool IsContextThread() const final;

  void uc2() override {
    printf("[Document::uc2 = %p]\n", this);
  }
};


class CORE_EXPORT HTMLDocument : public Document {
  DEFINE_WRAPPERTYPEINFO();
 public:
  explicit HTMLDocument(const long& = 12345);
  ~HTMLDocument() override;
 private:
  long some_fields;
};





long global_long = 12000;



ExecutionContext::ExecutionContext(int* isolate)
    : some_fields(12) {
    IsContextThread();
}

ExecutionContext::~ExecutionContext() = default;




Document::Document() : Document(global_long) {}

Document::Document(const long& initializer)
    : ExecutionContext(nullptr) {
      printf("document @ %p\n", this);
      debugExecutionContext();
      break_here();
    	IsContextThread();
      UseCounter::test(this);
}

Document::~Document() {
}

bool Document::IsContextThread() const {
	printf("[Document::IsContextThread %p]\n", this);
  return true;
}




HTMLDocument::HTMLDocument(const long& initializer)
    : Document(initializer) {
    	IsContextThread();
}

HTMLDocument::~HTMLDocument() = default;




}


int main() {
	new blink::HTMLDocument();
	return 0;
}