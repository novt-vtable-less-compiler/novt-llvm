#include "llvm/Transforms/IPO/NoVTAbiBuilder.h"

#include <utility>
#include <set>
#include <algorithm>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Format.h>
#include <queue>
#include <fstream>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Linker/Linker.h>

using namespace llvm;

#define with_debug_output 0
#define dout if (with_debug_output) llvm::errs()
#define advanced_loop_detection 0

#define seconds_since(start) (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count())

namespace {

    const bool BUILD_TRAPS = true;
    const bool BUILD_TRAPS_VBASE_OFFSET = false;
    const bool BUILD_NOINLINE = false;
    const bool BUILD_NOOPT_TRAMPOLINES = BUILD_NOINLINE && false;
    const bool BUILD_OPTIMIZE = true;
    const int BUILD_INSTRUMENTATION = 0; // 0=off, 1=lite, 2=full, 3=counting sizes


    std::chrono::high_resolution_clock::time_point compilerStart = std::chrono::high_resolution_clock::now();


    class DefClass;

    class DefClassIdent;

    std::vector<DefClass *> getInheritancePath(DefClassIdent *ident);

    raw_ostream &operator<<(raw_ostream &OS, const DefClassIdent &ident);

    struct InheritanceDefinition {
        DefClass *cls;
        int offset;
        int tmp_scratch; // used by different algorithms, non-permanent

        InheritanceDefinition(DefClass *cls, int offset) : cls(cls), offset(offset) {}
    };

    void parseIntToIntMap(std::map<int, int> &map, const std::string &idmap) {
      std::string buffer = idmap;
      size_t pos = 0;
      int key = 0;
      bool parseKey = true;
      while (!buffer.empty()) {
        if (parseKey) {
          key = std::stoi(buffer, &pos, 10);
        } else {
          map[key] = std::stoi(buffer, &pos, 10);
        }
        buffer = buffer.substr(pos + 1);
        parseKey = !parseKey;
      }
    }

    struct CVtable {
        std::vector<std::string> symbols;
        DefClass *base;
        DefClass *layout;
        int offset;
        std::map<int, int> cvtableIndexToLayoutClassOffset;

        CVtable(const std::string &symbol, DefClass *base, DefClass *layout, int offset, const std::string &idmap)
                : symbols({symbol}), base(base), layout(layout), offset(offset) {
          parseIntToIntMap(cvtableIndexToLayoutClassOffset, idmap);
        }
    };

    class DefClass {
    public:
        std::string name;
        std::vector<InheritanceDefinition> bases;
        std::vector<InheritanceDefinition> children;
        std::vector<InheritanceDefinition> vbases;
        std::vector<InheritanceDefinition> vchildren;
        std::vector<DefClassIdent *> idents;
        std::vector<DefClassIdent *> construction_idents;
        std::map<DefClass *, std::pair<int, int>> virtualIdentRanges; // [from, to) index in "idents"
        std::map<std::pair<std::string, int>, Function *> definitions; // (function name, thunk offset) => definition
        std::string vtableSymbol = "";
        std::map<int, int> vtableIdMap;
        std::vector<CVtable> cvtables;
        bool has_virtual_functions = false;

        explicit DefClass(std::string name) : name(std::move(name)) {}

        void addBase(DefClass *base, int offset) {
          for (auto b: bases) {
            if (b.cls == base)
              return;
          }
          bases.emplace_back(base, offset);
          base->children.emplace_back(this, -offset);
        }

        void addVBase(DefClass *base, int offset) {
          for (auto b: vbases) {
            if (b.cls == base)
              return;
          }
          vbases.emplace_back(base, offset);
          base->vchildren.emplace_back(this, -offset);
        }

        void sortThings() {
          auto comp = [](InheritanceDefinition d1, InheritanceDefinition d2) {
              if (d1.offset == d2.offset)
                return d1.tmp_scratch < d2.tmp_scratch;
              return d1.offset < d2.offset;
          };
          for (size_t i = 0; i < vbases.size(); i++) {
            vbases[i].tmp_scratch = i;
          }
          std::sort(vbases.begin(), vbases.end(), comp);
          for (size_t i = 0; i < bases.size(); i++) {
            bases[i].tmp_scratch = i;
          }
          std::sort(bases.begin(), bases.end(), comp);
        }

        void setVtable(const std::string &symbol, const std::string &idmap) {
          vtableSymbol = symbol;
          parseIntToIntMap(vtableIdMap, idmap);
        }

        /**
         * symbol = "base-in-layout"
         * @param symbol
         * @param base
         * @param layout
         * @param offset
         */
        void addCVTable(const std::string &symbol, DefClass *base, DefClass *layout, int offset,
                        const std::string &idmap) {
          for (auto &cvtable: cvtables) {
            if (cvtable.base == base && cvtable.layout == layout && cvtable.offset == offset) {
              cvtable.symbols.push_back(symbol);
              return;
            }
          }
          cvtables.emplace_back(symbol, base, layout, offset, idmap);
        }

        bool isVirtual() {
          if (!definitions.empty() || !vbases.empty() || has_virtual_functions)
            return true;
          for (auto &base: bases) {
            if (base.cls->isVirtual())
              return true;
          }
          return false;
        }

        /**
         *
         * @param funcname
         * @param path
         * @param offset offset from the current position in the class to this class beginning
         * @param layoutClass (optional) class used to calculate the layout of vbases (for cvtables)
         * @param offsetToLayoutClass offset from beginning of my current class (this) to the beginning of the layout class
         * @return
         */
        std::tuple<DefClass *, int, std::set<DefClass *>>
        getDefinitionInternal(const std::string &funcname, const std::vector<DefClass *> &path, int offset = 0,
                              DefClass *layoutClass = nullptr, int offsetToLayoutClass = 0) {
          // return type (nearest definition class, offset to this class, set of superseded definitions)
          std::vector<std::pair<DefClass *, int>> candidates;
          std::set<DefClass *> superseded;
          for (auto b: vbases) {
            auto baseOffset = b.offset;
            if (layoutClass) {
              baseOffset = offsetToLayoutClass + *layoutClass->getOffsetToBase(b.cls);
            }
            auto x = b.cls->getDefinitionInternal(funcname, path, offset + baseOffset, layoutClass,
                                                  offsetToLayoutClass - baseOffset);
            if (std::get<0>(x)) {
              candidates.emplace_back(std::get<0>(x), std::get<1>(x));
              for (auto c: std::get<2>(x)) superseded.insert(c);
            }
          }
          for (auto b: bases) {
            auto x = b.cls->getDefinitionInternal(funcname, path, offset + b.offset, layoutClass,
                                                  offsetToLayoutClass - b.offset);
            if (std::get<0>(x)) {
              candidates.emplace_back(std::get<0>(x), std::get<1>(x));
              for (auto c: std::get<2>(x)) superseded.insert(c);
            }
          }
          // if we override this function - supersede everything we saw so far
          auto it1 = definitions.find({funcname, offset});
          auto it2 = definitions.find({funcname, 0});
          if (it1 != definitions.end() || it2 != definitions.end()) {
            for (auto b: bases) superseded.insert(b.cls);
            for (auto b: vbases) superseded.insert(b.cls);
            return {this, offset, superseded};
          }
          // if we do not override this function - take the best candidate
          std::pair<DefClass *, int> bestCandidate = {nullptr, 0};
          int bestIndex = -1;
          for (const auto candidate: candidates) {
            if (superseded.find(candidate.first) == superseded.end()) {
              int index = std::find(path.begin(), path.end(), candidate.first) - path.begin();
              if (bestIndex < 0 || bestIndex > index) {
                bestCandidate = candidate;
                bestIndex = index;
              }
            }
          }

          return {bestCandidate.first, bestCandidate.second, superseded};
        }

        std::pair<Function *, int>
        getDefinition(const std::string &funcname, int offset, DefClassIdent *ident, DefClass *layoutClass = nullptr,
                      int layoutClassOffset = 0) {
          std::vector<DefClass *> inheritancePath = getInheritancePath(ident);
          auto result = getDefinitionInternal(funcname, inheritancePath, offset, layoutClass, layoutClassOffset);
          if (std::get<0>(result)) {
            auto cls = std::get<0>(result);
            auto cls_offset = std::get<1>(result);
            auto it = cls->definitions.find({funcname, cls_offset});
            if (it != cls->definitions.end())
              return {it->second, 0};
            it = cls->definitions.find({funcname, 0});
            if (it != cls->definitions.end())
              return {it->second, cls_offset};
          }
          return {nullptr, 0};
        }

        Optional<int> getOffsetToBase(DefClass *base) {
          if (base == this)
            return 0;
          for (auto b: vbases) {
            if (b.cls == base)
              return b.offset;
          }
          for (auto b: bases) {
            auto off = b.cls->getOffsetToBase(base);
            if (off) {
              return *off + b.offset;
            }
          }
          return {};
        }

        void print() {
          llvm::errs() << "class " << name << " : ";
          for (auto b: bases) llvm::errs() << b.cls->name << "(" << b.offset << ") ";
          for (auto b: vbases) llvm::errs() << "virtual " << b.cls->name << "(" << b.offset << ") ";
          llvm::errs() << "{\n";
          for (const auto &it: definitions)
            llvm::errs() << "    " << it.first.first << " +" << it.first.second << "\n";
          // llvm::errs() << "    " << it.first.first << " +" << it.first.second << "  (symbol " << it.second->getName() << ")\n";
          if (!vtableSymbol.empty()) {
            llvm::errs() << "    vtable " << vtableSymbol << "\n";
            llvm::errs() << "    vtable index map:  ";
            for (const auto &it: vtableIdMap) llvm::errs() << it.first << ":" << it.second << " ";
            llvm::errs() << "\n";
          }
          for (const auto ident: idents) {
            llvm::errs() << "    ident " << *ident << "\n";
          }
          for (const auto ident: construction_idents) {
            llvm::errs() << "    construction ident " << *ident << "\n";
          }
          llvm::errs() << "}\n";
        }

        Optional<DefClassIdent *> getIdentToOffset(int offset);
    };

    class DefClassIdent {
    public:
        DefClass *cls;
        int offsetToCls;
        // only for construction vtables
        DefClass *layout_cls = nullptr;
        int offsetToLayoutCls = 0;

        bool abstract = true;
        bool is_virtual = false;
        int vtable_index = 0;
        int base_index = 0;
        uint32_t number = 0xffffffff;
        std::set<DefClassIdent *> bases;
        std::set<DefClassIdent *> children;
        DefClassIdent *firstBase = nullptr;
        DefClassIdent *replacedBy = nullptr;
        bool removed = false;
        bool in_cluster = false;

        std::vector<User *> users;

        // storage for optimization passes
        std::set<DefClassIdent *> unequal;
        std::set<DefClassIdent *> equalCandidates;

        DefClassIdent(DefClass *cls, int offsetToCls, int vtableIndex, int baseIndex)
                : cls(cls), offsetToCls(offsetToCls), vtable_index(vtableIndex), base_index(baseIndex) {}

        DefClassIdent(DefClass *cls, int offsetToCls, int vtableIndex, int baseIndex, DefClass *layoutCls,
                      int offsetToLayoutCls)
                : cls(cls), offsetToCls(offsetToCls), layout_cls(layoutCls), offsetToLayoutCls(offsetToLayoutCls),
                  vtable_index(vtableIndex), base_index(baseIndex) {}

        void addChild(DefClassIdent *ident) {
          for (auto child: children) {
            if (child == ident) return;
          }
          this->children.insert(ident);
          ident->bases.insert(this);
          if (ident->firstBase == nullptr)
            ident->firstBase = this;
        }

        void replaceChild(DefClassIdent *identOld, DefClassIdent *identNew) {
          if (children.erase(identOld) > 0) {
            children.insert(identNew);
            identNew->bases.insert(this);
          }
        }

        void replaceBase(DefClassIdent *identOld, DefClassIdent *identNew) {
          if (bases.erase(identOld) > 0) {
            bases.insert(identNew);
            identNew->children.insert(this);
          }
          if (firstBase == identOld)
            firstBase = identNew;
        }

        Optional<DefClassIdent *> getBaseForClass(DefClass *cls) {
          for (auto &b: bases) {
            if (b->cls == cls)
              return b;
          }
          return {};
        }

        Optional<DefClassIdent *> getBaseForClassRecursive(DefClass *cls, bool only_virtuals = false,
                                                           std::set<DefClassIdent *> *additionalBases = nullptr) {
          Optional<DefClassIdent *> result;
          for (auto &b: bases) {
            if (b->cls == cls) {
              if (additionalBases) {
                if (!result)
                  result = b;
                additionalBases->insert(b);
              } else {
                return b;
              }
            }
          }
          for (auto &b: bases) {
            if (only_virtuals && !b->is_virtual) continue;
            auto opt = b->getBaseForClassRecursive(cls, only_virtuals, additionalBases);
            if (opt) {
              if (!additionalBases)
                return opt;
              else if (!result)
                result = opt;
            }
          }
          return result;
        }

        Optional<DefClassIdent *>
        getSimilarIdentForClass(const DefClass *cls, std::set<DefClassIdent *> *additionalBases) {
          std::set<DefClassIdent *> seen;
          std::queue<DefClassIdent *> queue;
          queue.push(this);
          seen.insert(this);
          Optional<DefClassIdent *> result;

          while (!queue.empty()) {
            auto ident = queue.front();
            queue.pop();
            if (ident->cls == cls && ident->layout_cls == nullptr) {
              if (!additionalBases)
                return ident;
              if (result)
                additionalBases->insert(ident);
              else
                result = ident;
            }
            for (auto i2: ident->children) {
              if (seen.find(i2) == seen.end()) {
                queue.push(i2);
                seen.insert(i2);
              }
            }
            for (auto i2: ident->bases) {
              if (seen.find(i2) == seen.end()) {
                queue.push(i2);
                seen.insert(i2);
              }
            }
          }

          return result;
        }

        void joinFrom(DefClassIdent *ident2) {
          assert(cls == ident2->cls);
          assert(offsetToCls == ident2->offsetToCls);
          assert(layout_cls == ident2->layout_cls);
          for (auto baseIdent: ident2->bases) {
            baseIdent->replaceChild(ident2, this);
          }
          for (auto childIdent: ident2->children) {
            childIdent->replaceBase(ident2, this);
          }
          abstract = abstract && ident2->abstract;
          users.insert(users.end(), ident2->users.begin(), ident2->users.end());
        }

        void toDot(llvm::raw_ostream &out) {
          out << "\"" << this << "\" [label=\"" << *this << "\"];\n";
          for (auto child: children) {
            out << "\"" << this << "\" -> \"" << child << "\";\n";
          }
        }

        int getNumber() {
          DefClassIdent *ident = this;
          while (ident->replacedBy) ident = ident->replacedBy;
          return ident->number;
        }

        void optReplaceBy(DefClassIdent *&ident) {
          replacedBy = ident;
          for (auto ue: unequal) {
            ident->unequal.insert(ue);
            ue->unequal.insert(ident);
          }
        }

        inline void remove() {
          removed = true;
        }
    };


    std::vector<DefClass *> getInheritancePath(DefClassIdent *ident) {
      std::vector<DefClass *> result;
      while (ident) {
        result.push_back(ident->cls);
        if (ident->bases.empty()) break;
        ident = ident->firstBase;
      }
      return result;
    }

    raw_ostream &operator<<(raw_ostream &OS, const DefClassIdent &ident) {
      OS << "#" << ident.vtable_index;
      if (ident.base_index != ident.vtable_index) OS << "/#" << ident.base_index;
      OS << " of " << ident.cls->name;
      if (ident.layout_cls) OS << " in " << ident.layout_cls->name;
      OS << ", cls@" << ident.offsetToCls;
      if (ident.layout_cls) OS << ", layoutCls@" << ident.offsetToLayoutCls;
      if (ident.is_virtual && ident.abstract) OS << " (virtual abstract)";
      else if (ident.is_virtual) OS << " (virtual)";
      else if (ident.abstract) OS << " (abstract)";
      if (ident.number != 0xffffffff) {
        OS << "  // ID " << ident.number << " (" << format_hex(ident.number, 1) << ")";
      }
      return OS;
    }

    Optional<DefClassIdent *> DefClass::getIdentToOffset(int offset) {
      for (auto ident: idents) {
        if (ident->offsetToCls == offset) {
          return ident;
        }
      }
      return {};
    }


    enum SwitchFunctionType {
        DISPATCHER = 0,
        VBASE_OFFSET = 1,
        DYNAMIC_CAST = 2,
        DYNAMIC_VOID_CAST = 3,
        RTTI_GET = 4
    };


    class SwitchFunction {
    public:
        const SwitchFunctionType type;
        Function *const function;
        SwitchInst *const switchInst;
        const bool defaultIsImportant;
        bool removed = false;

        std::map<DefClassIdent *, BasicBlock *> cases;
        std::set<BasicBlock *> blocks;
        std::set<DefClassIdent *> idents;
        std::set<DefClassIdent *> defaultIdents; // idents that go to the "default" case (if "defaultIsImportant")

        SwitchFunction(const SwitchFunctionType type, Function *function, SwitchInst *switchInst,
                       bool defaultIsImportant)
                : type(type), function(function), switchInst(switchInst), defaultIsImportant(defaultIsImportant) {}

        void addCase(DefClassIdent *ident, BasicBlock *block) {
          cases[ident] = block;
          blocks.insert(block);
          idents.insert(ident);
        }

        void addDefaultCase(DefClassIdent *ident) {
          defaultIdents.insert(ident);
        }

        void replaceSwitchInst(BasicBlock *bb) {
          IRBuilder<> builder((Instruction *) switchInst);
          builder.CreateBr(bb);
          switchInst->eraseFromParent();
          function->removeFnAttr(Attribute::NoInline);
          function->removeFnAttr(Attribute::OptimizeNone);
          function->addFnAttr(Attribute::AlwaysInline);
          cases.clear();
          blocks.clear();
          idents.clear();
          removed = true;
        }

        void buildCases() {
          if (removed)
            return;
          auto IntPtr = cast<IntegerType>(function->getParent()->getDataLayout().getIntPtrType(function->getContext()));
          for (const auto it: cases) {
            switchInst->addCase(ConstantInt::get(IntPtr, it.first->number), it.second);
          }
        }

        void assertIdentNumbers() {
          if (removed)
            return;
          std::set<uint32_t> seenNumbers;
          for (const auto it: cases) {
            assert(it.first->number != 0xffffffff);
            auto result = seenNumbers.insert(it.first->number);
            assert(result.second);
          }
        }
    };


    class AbiBuilder {
        Module &M;
        LLVMContext &C;
        std::map<std::string, std::unique_ptr<DefClass>> classes;
        std::vector<std::unique_ptr<DefClassIdent>> idents;
        std::vector<std::unique_ptr<SwitchFunction>> createdFunctions;
        IntegerType *Int32;
        Type *IntPtr;
        PointerType *Int8Ptr;

        long count_dispatchers = 0;
        long count_dispatcher_usages = 0;
        long count_rtti = 0;
        long count_dynamic_cast = 0;
        long count_dynamic_cast_void = 0;
        long count_vbase_offset_getter = 0;
    public:
        explicit AbiBuilder(Module &m) : M(m), C(M.getContext()), Int32(Type::getInt32Ty(M.getContext())),
                                         IntPtr(m.getDataLayout().getIntPtrType(M.getContext())),
                                         Int8Ptr(Type::getInt8PtrTy(M.getContext())) {}

        DefClass *getClass(const std::string &name) {
          auto it = classes.find(name);
          if (it != classes.end())
            return it->second.get();
          return classes.insert({name, std::make_unique<DefClass>(name)}).first->second.get();
        }

        static bool isConstant(Value *v, uint64_t c) {
          auto x = dyn_cast<ConstantInt>(v);
          if (!x) return false;
          return x->getValue().getZExtValue() == c;
        }

        static uint64_t getConstant(Value *v, uint64_t def = -1) {
          auto x = dyn_cast<ConstantInt>(v);
          if (!x) return def;
          return x->getValue().getZExtValue();
        }

        /**
         * Parse the metadata from LLVM and initialize all these "Class" structs
         */
        void buildClassHierarchy() {
          for (auto &md: M.getNamedMDList()) {
            if (!md.getName().startswith("novt.")) continue;
            if (md.getName().endswith(".bases")) {
              auto cls = getClass(md.getName().substr(5, md.getName().size() - 11));
              for (unsigned i = 0; i < md.getNumOperands(); i++) {
                if (md.getOperand(i)->getNumOperands() > 1) {
                  auto basename = cast<MDString>(md.getOperand(i)->getOperand(0))->getString();
                  auto offset = std::stoi(cast<MDString>(md.getOperand(i)->getOperand(1))->getString());
                  cls->addBase(getClass(basename), offset);
                }
              }

            } else if (md.getName().endswith(".vbases")) {
              auto cls = getClass(md.getName().substr(5, md.getName().size() - 12));
              for (unsigned i = 0; i < md.getNumOperands(); i++) {
                if (md.getOperand(i)->getNumOperands() > 1) {
                  auto basename = cast<MDString>(md.getOperand(i)->getOperand(0))->getString();
                  auto offset = std::stoi(cast<MDString>(md.getOperand(i)->getOperand(1))->getString());
                  cls->addVBase(getClass(basename), offset);
                }
              }

            } else if (md.getName().endswith(".methods")) {
              auto cls = getClass(md.getName().substr(5, md.getName().size() - 13));
              for (unsigned i = 0; i < md.getNumOperands(); i++) {
                if (md.getOperand(i)->getNumOperands() > 0) {
                  // Format: {function name, function symbol, ["thunkoffset:thunksymbol"]...}
                  auto funcname = cast<MDString>(md.getOperand(i)->getOperand(0))->getString();
                  auto funcRef = dyn_cast_or_null<ValueAsMetadata>(md.getOperand(i)->getOperand(2));
                  if (funcRef && funcRef->getValue()) {
                    auto f = dyn_cast<Function>(funcRef->getValue());
                    if (f)
                      cls->definitions.insert({{funcname, 0}, f});
                  } else {
                    auto implname = cast<MDString>(md.getOperand(i)->getOperand(1))->getString();
                    auto f = M.getFunction(implname);
                    if (!f && funcname.equals("D1v") && implname.endswith("D2Ev")) {
                      std::string newName = implname;
                      newName[newName.size() - 3] = '1';
                      f = M.getFunction(newName);
                    }
                    if (f) {
                      cls->definitions.insert({{funcname, 0}, f});
                    } else {
                      dout << "[NOTFOUND] Could not find function " << implname << " (from " << *md.getOperand(i)
                           << " class " << cls->name << ")\n";
                    }
                  }
                  // parse thunk definitions for this method
                  int thunkOffset = 0;
                  for (unsigned int j = 3; j < md.getOperand(i)->getNumOperands(); j++) {
                    auto thunkspec = dyn_cast_or_null<MDString>(md.getOperand(i)->getOperand(j));
                    if (thunkspec) {
                      thunkOffset = std::stoi(thunkspec->getString());
                    } else {
                      auto thunkFunction = dyn_cast_or_null<ValueAsMetadata>(md.getOperand(i)->getOperand(j));
                      if (thunkFunction && thunkFunction->getValue()) {
                        auto f = dyn_cast<Function>(thunkFunction->getValue());
                        if (f)
                          cls->definitions.insert({{funcname, thunkOffset}, f});
                      } else {
                        // llvm::errs() << "Could not find thunk " << thunkspec << " (from " << *md.getOperand(i) << ")\n";
                      }
                    }
                  }
                  cls->has_virtual_functions = true;
                }
              }

            } else if (md.getName().endswith(".vtable")) {
              auto cls = getClass(md.getName().substr(5, md.getName().size() - 12));
              for (unsigned int i = 0; i < md.getNumOperands(); i++) {
                std::string idmap;
                if (md.getOperand(i)->getNumOperands() > 1) {
                  idmap = cast<MDString>(md.getOperand(i)->getOperand(1))->getString();
                }
                const auto &op0 = md.getOperand(i)->getOperand(0);
                auto vtable = dyn_cast_or_null<ValueAsMetadata>(op0);
                if (vtable) {
                  assert(vtable->getValue() && "vtable symbol without value");
                  cls->setVtable(vtable->getValue()->getName(), idmap);
                  break;
                } else if (op0) {
                  cls->setVtable(cast<MDString>(op0)->getString(), idmap);
                }
              }
              // dout << "class " << md.getName().substr(5, md.getName().size() - 12) << " has vtable named "
              //      << vtable << "  '" << md.getName() << "'" << "\n";

            } else if (md.getName().endswith(".cvtables")) {
              auto cls = getClass(md.getName().substr(5, md.getName().size() - 14));

              for (unsigned i = 0; i < md.getNumOperands(); i++) {
                if (md.getOperand(i)->getNumOperands() > 1) {
                  auto symbolStr = cast<MDString>(md.getOperand(i)->getOperand(0))->getString();
                  auto base = getClass(cast<MDString>(md.getOperand(i)->getOperand(1))->getString());
                  auto layout = getClass(cast<MDString>(md.getOperand(i)->getOperand(2))->getString());
                  auto offset = std::stoi(cast<MDString>(md.getOperand(i)->getOperand(3))->getString());
                  auto idMapString = dyn_cast<MDString>(md.getOperand(i)->getOperand(4));
                  if (md.getOperand(i)->getNumOperands() > 5 && md.getOperand(i)->getOperand(5)) {
                    auto symbolGlobal = dyn_cast_or_null<ValueAsMetadata>(md.getOperand(i)->getOperand(5));
                    if (symbolGlobal && symbolGlobal->getValue())
                      symbolStr = symbolGlobal->getValue()->getName();
                  }
                  cls->addCVTable(symbolStr, base, layout, offset, idMapString ? idMapString->getString() : "");
                }
              }
            }
          }
          for (const auto &cls: classes) {
            cls.second->sortThings();
          }
        }

        void printClasses() {
          llvm::errs() << "\n\n";
          for (auto &it: classes) {
            it.second->print();
          }
          llvm::errs() << "\n\n";
        }

        void printIdents() {
          llvm::errs() << "\n\n";
          llvm::errs() << "digraph G{\n";
          for (auto &it: classes) {
            for (auto ident: it.second->idents) {
              ident->toDot(llvm::errs());
            }
            for (auto ident: it.second->construction_idents) {
              ident->toDot(llvm::errs());
            }
          }
          llvm::errs() << "}\n";
          llvm::errs() << "\n\n";
        }

        /**
         * Add identifiers (numbers) to all classes
         */
        void buildClassNumbers() {
          uint32_t i = 0;
          for (auto &it: classes) {
            for (auto ident: it.second->idents) {
              if (!ident->abstract)
                ident->number = i++;
            }
            for (auto ident: it.second->construction_idents) {
              if (!ident->abstract)
                ident->number = i++;
            }
          }
        }

        std::set<DefClass *> loopDetectionClasses;

        void buildIdentsForClass(DefClass *cls) {
          if (!cls->idents.empty())
            return;
          if (advanced_loop_detection) {
            auto it = loopDetectionClasses.insert(cls);
            if (!it.second) {
              llvm::errs() << "This class is part of a loop: " << cls->name << "\n";
            }
          }
          int next_ident_index = 0;
          for (auto it: cls->bases) {
            buildIdentsForClass(it.cls);
            for (auto baseIdent: it.cls->idents) {
              if (!baseIdent->is_virtual) {
                auto index = next_ident_index++;
                auto new_ident = new DefClassIdent(cls, -it.offset + baseIdent->offsetToCls, index, index);
                idents.emplace_back(new_ident);
                cls->idents.push_back(new_ident);
                baseIdent->addChild(new_ident);
              }
            }
          }
          for (auto it: cls->bases) {
            // Copy over virtual idents from this base
            for (auto it2: it.cls->vbases) {
              if (cls->virtualIdentRanges.find(it2.cls) == cls->virtualIdentRanges.end()) {
                auto start_index = next_ident_index;
                auto startstop = it.cls->virtualIdentRanges[it2.cls];
                for (int i = startstop.first; i < startstop.second; i++) {
                  auto index = next_ident_index++;
                  idents.emplace_back(new DefClassIdent(cls, 0, index, index));
                  auto new_ident = idents[idents.size() - 1].get();
                  cls->idents.push_back(new_ident);
                  it.cls->idents[i]->addChild(new_ident);
                  new_ident->is_virtual = true;
                }
                cls->virtualIdentRanges[it2.cls] = {start_index, next_ident_index};
              }
            }
          }

          // Give this class at least one non-virtual ident (if it is virtual)
          if (cls->idents.empty() && cls->isVirtual()) {
            auto index = next_ident_index++;
            idents.emplace_back(new DefClassIdent(cls, 0, index, index));
            cls->idents.push_back(idents[idents.size() - 1].get());
          }

          // Create virtual idents for new bases
          for (auto it: cls->vbases) {
            if (cls->virtualIdentRanges.find(it.cls) == cls->virtualIdentRanges.end()) {
              buildIdentsForClass(it.cls);
              auto start_index = next_ident_index;
              for (auto baseIdent: it.cls->idents) {
                if (!baseIdent->is_virtual) {
                  auto index = next_ident_index++;
                  idents.emplace_back(new DefClassIdent(cls, 0, index, index));
                  auto new_ident = idents[idents.size() - 1].get();
                  cls->idents.push_back(new_ident);
                  baseIdent->addChild(new_ident);
                  new_ident->is_virtual = true;
                }
              }
              cls->virtualIdentRanges[it.cls] = {start_index, next_ident_index};
            }
          }

          // Compute virtual offsets
          for (auto it: cls->vbases) {
            if (cls->virtualIdentRanges.find(it.cls) != cls->virtualIdentRanges.end()) {
              auto startstop = cls->virtualIdentRanges[it.cls];
              for (int i = startstop.first; i < startstop.second; i++) {
                cls->idents[i]->offsetToCls = -it.offset + it.cls->idents[i - startstop.first]->offsetToCls;
              }
            }
          }

          if (advanced_loop_detection) {
            loopDetectionClasses.erase(cls);
          }
        }

        void buildIdentHierarchy() {
          // build identifiers
          for (auto &it: classes) {
            // dout << "buildIdentsForClass(" << it.second->name << ") ...\n";
            buildIdentsForClass(it.second.get());
          }
          dout << "all idents built, now postprocessing ...\n";
          // join identifiers where only one vtable exists (classes without data)
          for (auto &it: classes) {
            auto &classIdents = it.second->idents;
            int sub = 0;
            std::map<int, int> offsetToIndex;
            for (size_t i = 0; i < classIdents.size();) {
              classIdents[i]->vtable_index -= sub;
              auto offsetAlreadyTaken = offsetToIndex.find(classIdents[i]->offsetToCls);
              if (offsetAlreadyTaken != offsetToIndex.end()) {
                classIdents[offsetAlreadyTaken->second]->joinFrom(classIdents[i]);
                classIdents.erase(classIdents.begin() + i);
                sub++;
              } else {
                offsetToIndex[classIdents[i]->offsetToCls] = i;
                i++;
              }
            }
          }
          // Iterate all vtables and set corresponding identifiers non-abstract / add users
          for (auto &it: classes) {
            auto vtable = M.getNamedGlobal(it.second->vtableSymbol);
            if (vtable) {
              auto numVtables = vtable->getType()->getPointerElementType()->getStructNumElements();
              // error handling
              if (numVtables != it.second->idents.size()) {
                printClasses();
                llvm::errs() << "Class " << it.second->name << "\n";
                llvm::errs() << "vtables: " << numVtables << " (" << *vtable->getType() << ")\n";
                llvm::errs() << "idents: " << it.second->idents.size() << "\n";
                for (auto ident: it.second->idents) {
                  llvm::errs() << "- ident for " << ident->cls->name << " " << ident->abstract << ident->is_virtual
                               << "\n";
                }
              }
              assert(numVtables == it.second->idents.size() && "VTable count did not match identifier count");
              // end error handling
              for (auto user: vtable->users()) {
                // store i32 (...)** bitcast (i8** getelementptr inbounds ({ [5 x i8*] }, { [5 x i8*] }* @_ZTV1A, i64 0, inrange i32 0, i64 2) to i32 (...)**), i32 (...)*** %2, align 8, !tbaa !26
                auto gep = dyn_cast<GEPOperator>(user);
                if (gep && gep->getNumOperands() == 4 && isConstant(gep->getOperand(1), 0) &&
                    getConstant(gep->getOperand(2)) < numVtables && gep->getNumUses() > 0) {
                  auto numUsedVtable = getConstant(gep->getOperand(2));
                  // get the correct ident
                  auto tableIdent = it.second->idents[numUsedVtable];
                  auto offsetToClass = it.second->vtableIdMap.find(numUsedVtable);
                  if (offsetToClass != it.second->vtableIdMap.end()) {
                    auto tableIdentOpt = it.second->getIdentToOffset(-offsetToClass->second);
                    if (!tableIdentOpt) {
                      llvm::errs() << "[ERR] No ident found for offset " << offsetToClass->second << " in class "
                                   << it.second->name << "\n";
                    } else {
                      tableIdent = *tableIdentOpt;
                    }
                  }
                  tableIdent->abstract = false;
                  tableIdent->users.push_back(user);
                } else {
                  llvm::errs() << "[WARNING] Can't remove usage of vtable " << *user << "\n";
                }
              }
            }

            for (const auto &cvtable: it.second->cvtables) {
              dout << " --- new cvtable ---  // and offsetCurrentClassToLayout for grep\n";
              int offsetCurrentClassToLayout = -cvtable.offset;
              dout << "Start with offsetCurrentClassToLayout = " << offsetCurrentClassToLayout << "\n";
              for (auto &symbol: cvtable.symbols) {
                vtable = M.getNamedGlobal(symbol);
                if (vtable) {
                  std::map<int, DefClassIdent *> usedTables;
                  auto numVtables = vtable->getType()->getPointerElementType()->getStructNumElements();
                  for (auto user: vtable->users()) {
                    // store i32 (...)** bitcast (i8** getelementptr inbounds ({ [5 x i8*] }, { [5 x i8*] }* @_ZTV1A, i64 0, inrange i32 0, i64 2) to i32 (...)**), i32 (...)*** %2, align 8, !tbaa !26
                    auto gep = dyn_cast<GEPOperator>(user);
                    if (gep && gep->getNumOperands() == 4 && isConstant(gep->getOperand(1), 0) &&
                        getConstant(gep->getOperand(2)) < numVtables && gep->getNumUses() > 0) {
                      auto numOfUsedVtable = getConstant(gep->getOperand(2));
                      if (usedTables.count(numOfUsedVtable) == 0) {
                        // resolve shit - get the parent ident from the given offset.
                        // we end up with the ident that should store the vtable - from the "base" class.
                        // we use the layout class ident as fallback.
                        dout << "CVtable index " << numOfUsedVtable << "\n";
                        auto offset = cvtable.cvtableIndexToLayoutClassOffset.find(numOfUsedVtable);
                        auto baseIdent = numOfUsedVtable < cvtable.base->idents.size()
                                         ? cvtable.base->idents[numOfUsedVtable] : nullptr;
                        std::set<DefClassIdent *> additionalBases;
                        auto offsetInLayout = -cvtable.offset;
                        auto offsetInCurrent = baseIdent ? baseIdent->offsetToCls : 0;
                        if (offset == cvtable.cvtableIndexToLayoutClassOffset.end()) {
                          llvm::errs() << "[ERRS] No offset found for cvtable " << cvtable.base->name << " in "
                                       << cvtable.layout->name << ": index " << numOfUsedVtable << "\n";
                        } else {
                          auto identInLayout = cvtable.layout->getIdentToOffset(-offset->second);
                          if (!identInLayout) {
                            llvm::errs() << "[ERRS] No offset ident found for cvtable " << cvtable.base->name << " in "
                                         << cvtable.layout->name << ": index " << numOfUsedVtable << " offset "
                                         << offset->second << " + " << cvtable.offset << "\n";
                          } else {
                            offsetInLayout = (*identInLayout)->offsetToCls;
                            auto baseIdentOpt = (*identInLayout)->getBaseForClassRecursive(cvtable.base,
                                                                                           (*identInLayout)->is_virtual,
                                                                                           &additionalBases);
                            if (!baseIdentOpt) {
                              baseIdentOpt = (*identInLayout)->getSimilarIdentForClass(cvtable.base, &additionalBases);
                              if (baseIdentOpt)
                                dout << "getBaseForClassRecursive did not find, but baseIdentOpt found = "
                                     << **baseIdentOpt << "\n";
                            }
                            if (!baseIdentOpt) {
                              llvm::errs() << "[ERRS] No base ident found for cvtable " << cvtable.base->name << " in "
                                           << cvtable.layout->name << ": index " << numOfUsedVtable << " offset "
                                           << offset->second << " + " << cvtable.offset << " layout-ident "
                                           << **identInLayout << "\n";
                              baseIdent = *identInLayout;
                            } else {
                              baseIdent = *baseIdentOpt;
                              if ((*identInLayout)->is_virtual || true) {
                                offsetInCurrent = offsetInLayout - offsetCurrentClassToLayout;
                                dout << "using offsetCurrentClassToLayout=" << offsetCurrentClassToLayout
                                     << " set offsetInCurrent to " << offsetInCurrent << " - identInLayout="
                                     << **identInLayout << "    v=" << (*identInLayout)->is_virtual << "\n";
                              } else {
                                /*offsetInCurrent = baseIdent->offsetToCls;
                                offsetCurrentClassToLayout = offsetInLayout - offsetInCurrent;
                                dout << "offsetCurrentClassToLayout = " << offsetCurrentClassToLayout
                                     << " for cvtable (because " << offsetInLayout << " - (" << offsetInCurrent
                                     << ") identInLayout=" << **identInLayout << "\n";*/
                              }
                            }
                          }
                        }
                        assert(baseIdent && "no base ident found");
                        dout << "baseIdent = " << *baseIdent << "       layoutclass = " << cvtable.layout->name
                             << " // offsetCurrentClassToLayout\n";

                        auto new_ident = new DefClassIdent(cvtable.base, offsetInCurrent, numOfUsedVtable,
                                                           baseIdent->base_index, cvtable.layout, offsetInLayout);
                        idents.emplace_back(new_ident);
                        baseIdent->addChild(new_ident);
                        new_ident->abstract = false;
                        new_ident->is_virtual = baseIdent->is_virtual;
                        cvtable.base->construction_idents.push_back(new_ident);
                        for (auto additionalBase: additionalBases) {
                          additionalBase->addChild(new_ident);
                        }
                        usedTables[numOfUsedVtable] = new_ident;
                        dout << " => " << *new_ident << "\n";
                      }
                      usedTables[numOfUsedVtable]->users.push_back(user);
                    } else {
                      llvm::errs() << "[WARNING] Can't remove usage of construction vtable " << *user << "\n";
                    }
                  }
                }
              }
            }
          }
        }


        template<class CacheType>
        struct FunctionBuilder {
        private:
            std::map<CacheType, BasicBlock *> cache;
        public:
            const SwitchFunctionType type;
            Module &M;
            LLVMContext &C;
            Function *function;
            int thisArgNumber = 0;
            SwitchInst *switchInst = nullptr;
            std::set<DefClassIdent *> visitedIdents;
            SwitchFunction *swfunc = nullptr;
            bool defaultIsImportant = false;

            explicit FunctionBuilder(const SwitchFunctionType type, Function *function)
                    : type(type), M(*function->getParent()), C(function->getContext()), function(function) {
              if (BUILD_NOINLINE) {
                function->addFnAttr(Attribute::NoInline);
              }
              if (BUILD_NOOPT_TRAMPOLINES) {
                function->addFnAttr(Attribute::OptimizeNone);
              }
              setCallingConvention();
            }

            void setCallingConvention() {
              for (auto user: function->users()) {
                auto call = dyn_cast<CallInst>(user);
                if (call && call->getCalledFunction() && call->getCalledFunction() == function) {
                  function->setCallingConv(call->getCallingConv());
                  break;
                }
                auto invoke = dyn_cast<InvokeInst>(user);
                if (invoke && invoke->getCalledFunction() && invoke->getCalledFunction() == function) {
                  function->setCallingConv(invoke->getCallingConv());
                  break;
                }
              }
            }

            inline Type *Int32() {
              return IntegerType::getInt32Ty(C);
            }

            inline Type *Int8Ptr() {
              return IntegerType::getInt8PtrTy(C);
            }

            inline Type *IntPtr() {
              return M.getDataLayout().getIntPtrType(M.getContext());
            }

            static inline std::string getCaseName(DefClassIdent *ident) {
              return "case_" + ident->cls->name + "_" + std::to_string(ident->number);
            }

            bool isCached(CacheType ct) {
              return cache.find(ct) != cache.end();
            }

            BasicBlock *getCached(CacheType ct) {
              auto it = cache.find(ct);
              return it == cache.end() ? nullptr : it->second;
            }

            BasicBlock *setCached(CacheType ct, BasicBlock *bb) {
              cache[ct] = bb;
              return bb;
            }

            // override:
            // BasicBlock* createCaseBlock(DefClassIdent* ident, bool isRoot, BasicBlock* parentBlock);

            void buildDefaultCase(BasicBlock *defaultCaseBlock) {
              IRBuilder<> builder(defaultCaseBlock);
              if (BUILD_TRAPS) {
                builder.CreateIntrinsic(Intrinsic::trap, {}, {});
              }
              builder.CreateUnreachable();
            }

            Value *buildIdentLoad(BasicBlock *block) {
              assert((size_t) thisArgNumber < function->arg_size());
              auto thisarg = function->arg_begin();
              std::advance(thisarg, thisArgNumber);
              IRBuilder<> builder(block);
              auto ptrToVtableCasted = builder.CreateBitOrPointerCast(thisarg, IntPtr()->getPointerTo());
              return builder.CreateAlignedLoad(ptrToVtableCasted, M.getDataLayout().getPointerPrefAlignment());
            }
        };

        template<class T>
        void buildFunction(T &t, DefClass *staticType) {
          assert(staticType);
          t.function->setLinkage(Function::LinkageTypes::InternalLinkage);

          auto block = BasicBlock::Create(C, "entry", t.function);
          auto defaultCase = BasicBlock::Create(C, "defaultCase", t.function);
          auto vtableIndex = t.buildIdentLoad(block);

          IRBuilder<> builder(block);
          if (BUILD_INSTRUMENTATION == 1) {
            auto ft = FunctionType::get(Type::getVoidTy(C), {Int32}, false);
            builder.CreateCall(M.getOrInsertFunction("__instrument_count_lite", ft),
                               {ConstantInt::get(Int32, t.type, false)});
          } else if (BUILD_INSTRUMENTATION == 2) {
            auto ft = FunctionType::get(Type::getVoidTy(C), {Int8Ptr}, false);
            builder.CreateCall(M.getOrInsertFunction("__instrument_count", ft),
                               {builder.CreateGlobalStringPtr(t.function->getName())});
          }
          t.switchInst = builder.CreateSwitch(vtableIndex, defaultCase);
          createdFunctions.emplace_back(new SwitchFunction(t.type, t.function, t.switchInst, t.defaultIsImportant));
          t.swfunc = createdFunctions.back().get();
          if (staticType->idents.empty()) {
            llvm::errs() << "NO IDENTS: " << staticType->name << " (function " << t.function->getName() << ")\n";
          }
          assert(!staticType->idents.empty());
          buildFunctionCase(t, staticType->idents[0], true, nullptr);
          t.buildDefaultCase(defaultCase);
        }

        template<class T>
        void buildFunctionCase(T &t, DefClassIdent *ident, bool isRoot, BasicBlock *parentBlock) {
          if (!ident->abstract) {
            BasicBlock *block = t.createCaseBlock(ident, isRoot, parentBlock);
            if (block) {
              // t.switchInst->addCase(ConstantInt::get(IntPtr, ident->number), block);
              t.visitedIdents.insert(ident);
              t.swfunc->addCase(ident, block);
              parentBlock = block;
            } else if (t.swfunc->defaultIsImportant) {
              t.swfunc->addDefaultCase(ident);
            }
          }
          for (auto identChild: ident->children) {
            if (t.visitedIdents.find(identChild) == t.visitedIdents.end()) {
              buildFunctionCase(t, identChild, false, parentBlock);
            }
          }
        }


        /**
         * Collect a list of all possible virtual functions that can be called by a given dispatcher
         * @param targets
         * @param cls The class we dispatch on
         * @param functionName the function we will dispatch
         * @param offset
         * @param recursive
         */
        void collectCallTargets(std::set<Function *> &targets, DefClass *cls, const std::string &functionName,
                                int offset = 0) {
          auto func = cls->getDefinition(functionName, offset, nullptr);
          if (func.first) {
            targets.insert(func.first);
          }
          for (auto child: cls->children) {
            collectCallTargets(targets, child.cls, functionName, offset + child.offset);
          }
          for (auto child: cls->vchildren) {
            collectCallTargets(targets, child.cls, functionName, offset + child.offset);
          }
        }


        /**
         * Replace vtable pointer in constructor invocation with class constant
         */
        void patchConstructors() {
          for (auto &ident: idents) {
            if (!ident->abstract) {
              for (auto identUser: ident->users) {
                auto num = ConstantInt::get(IntPtr, ident->getNumber());
                typesafeReplaceAllUsesWith(identUser, num);
              }
            }
          }
        }

        static Constant *constantCast(Constant *newValue, Type *type) {
          if (type == newValue->getType()) {
            return newValue;
          } else if (type->isPointerTy()) {
            if (newValue->getType()->isPointerTy()) {
              return ConstantExpr::getBitCast(newValue, type);
            } else if (newValue->getType()->isIntegerTy()) {
              return ConstantExpr::getIntToPtr(newValue, type);
            } else {
              return nullptr;
            }
          } else if (type->isIntegerTy()) {
            if (newValue->getType()->isPointerTy()) {
              return ConstantExpr::getPtrToInt(newValue, type);
            } else if (newValue->getType()->isIntegerTy()) {
              return ConstantExpr::getIntegerCast(newValue, type, true);
            } else {
              return nullptr;
            }
          }
          return nullptr;
        }

        static bool typesafeReplaceAllUsesWith(User *oldValue, Constant *newValue) {
          std::vector<User *> users(oldValue->user_begin(), oldValue->user_end());
          bool result = true;
          for (auto user: users) {
            if (isa<BitCastOperator>(user) || isa<PtrToIntOperator>(user)) {
              // User is already a cast
              auto castedNewValue = constantCast(newValue, user->getType());
              assert(castedNewValue != nullptr && "Invalid conversion (1)");
              user->replaceAllUsesWith(castedNewValue);
            } else if (isa<ConstantStruct>(user)) {
              // oldValue is part of a constant struct
              auto const_struct = dyn_cast<ConstantStruct>(user);
              std::vector<Constant *> fields;
              for (unsigned i = 0; i < const_struct->getNumOperands(); i++) {
                auto op = const_struct->getOperand(i);
                if (op && op == dyn_cast<Constant>(oldValue)) {
                  auto castedNewValue = constantCast(newValue, op->getType());
                  assert(castedNewValue != nullptr && "Invalid conversion (2)");
                  fields.push_back(castedNewValue);
                } else {
                  fields.push_back(op);
                }
              }
              const_struct->replaceAllUsesWith(ConstantStruct::get(const_struct->getType(), fields));
            } else {
              llvm::errs() << "[WARNING] cannot handle vtable / placeholder usage: " << *user << "\n";
              result = false;
            }
          }
          return result;
        }

        void buildCases() {
          for (auto &sf: createdFunctions) {
            sf->assertIdentNumbers();
            sf->buildCases();
          }
        }

        /**
         *
         * @param f the dispatcher we define
         * @param classname
         * @param function
         * @return number of the "this" argument
         */
        int setDispatcherArgumentParameters(Function &f, StringRef &classname, StringRef &function) {
          std::set<Function *> callTargets;
          collectCallTargets(callTargets, getClass(classname), function, 0);
          if (callTargets.empty()) {
            llvm::errs() << "[WARN] No call targets for function " << f.getName() << " (" << classname << " " <<
                         function << ")\n";
            // llvm::report_fatal_error("No call targets for a virtual function.");
            return -1;
          }
          std::vector<AttributeSet> attributes;
          for (size_t i = 0; i < f.arg_size(); i++) {
            attributes.push_back((*callTargets.begin())->getAttributes().getParamAttributes(i));
          }
          for (auto vf: callTargets) {
            for (size_t i = 0; i < std::min(vf->arg_size(), attributes.size()); i++) {
              AttributeSet vfa = vf->getAttributes().getParamAttributes(i);
              for (auto a: vfa) {
                auto kind = a.getKindAsEnum();
                // remove changed attributes
                if (kind != Attribute::AttrKind::ByVal && kind !=
                                                          Attribute::AttrKind::StructRet) {  // these attribute are either present or not, we don't care if types don't match
                  if (attributes[i].hasAttribute(kind)) {
                    if (a != attributes[i].getAttribute(kind)) {
                      attributes[i] = attributes[i].removeAttribute(C, a.getKindAsEnum());
                    }
                  }
                }
              }
              auto currentAttributeSet = attributes[i];
              for (auto a: currentAttributeSet) {
                if (!vfa.hasAttribute(a.getKindAsEnum())) {
                  attributes[i] = attributes[i].removeAttribute(C, a.getKindAsEnum());
                }
              }
            }
          }

          // copy over certain attributes
          for (size_t i = 0; i < attributes.size(); i++) {
            if (attributes[i].hasAttribute(Attribute::AttrKind::ByVal))
              f.addParamAttr(i, Attribute::AttrKind::ByVal);
            if (attributes[i].hasAttribute(Attribute::AttrKind::StructRet))
              f.addParamAttr(i, Attribute::AttrKind::StructRet);
          }

          for (size_t i = 0; i < attributes.size(); i++) {
            if (!attributes[i].hasAttribute(Attribute::AttrKind::StructRet))
              return i;
          }
          return 0;
        }

        static llvm::Value *createCast(llvm::IRBuilder<> &builder, llvm::Value *value, llvm::Type *type) {
          assert(value && "value is null");
          assert(value->getType() && "value->getType() is null!");
          assert(type && "type is null");
          if (value->getType() == type)
            return value;
          if (value->getType()->isIntOrPtrTy() && type->isIntOrPtrTy()) {
            return builder.CreateBitOrPointerCast(value, type);
          }
          if (value->getType()->isStructTy() && type->isStructTy()) {
            Value *result = UndefValue::get(type);
            for (unsigned int i = 0; i < value->getType()->getStructNumElements(); i++) {
              Value *v = builder.CreateExtractValue(value, {i});
              v = createCast(builder, v, type->getStructElementType(i));
              result = builder.CreateInsertValue(result, v, {i});
            }
            return result;
          }
          llvm::errs() << "VALUE = " << *value << "\n";
          llvm::errs() << "Type  = " << *type << "\n";
          llvm::report_fatal_error("Invalid cast requested");
        }

        void defineVirtualDispatcher(Function &f) {
          auto idx = f.getName().find_first_of(".", 7);
          if (idx == StringRef::npos) return;
          auto classname = f.getName().substr(7, idx - 7);
          auto function = f.getName().substr(idx + 1);
          dout << "define: " << f.getName() << " class " << classname << " func " << function << " used "
               << f.getNumUses() << "\n";
          if (classes.count(classname) == 0) {
            // There is no class that could be used to invoke this method. OR: this is an error.
            llvm::errs() << "[ERR] Class not found: " << classname << "\n";
            createBrokenFunction(f);
            return;
          }

          // get information about argument modifiers (we keep modifiers if they are present in all virtual targets)
          auto thisArgNumber = setDispatcherArgumentParameters(f, classname, function);
          if (thisArgNumber == -1) {
            // this function can't be called (???)
            createBrokenFunction(f);
            return;
          }

          struct DispatcherBuilder : public FunctionBuilder<std::pair<Function *, int>> {
              StringRef functionName;

              explicit DispatcherBuilder(Function *function) : FunctionBuilder(DISPATCHER, function) {}

              BasicBlock *createCaseBlock(DefClassIdent *ident, bool isRoot, BasicBlock *parentBlock) {
                auto offset = ident->offsetToCls;
                auto layoutOffset = 0;
                if (ident->layout_cls) {
                  //layoutOffset = -*ident->layout_cls->getOffsetToBase(ident->cls);
                  //offset = ident->offsetToLayoutCls - layoutOffset;
                  layoutOffset = ident->offsetToLayoutCls - ident->offsetToCls;
                  //llvm::errs() << "calculate offset: " << ident->offsetToLayoutCls << " + " << -layoutOffset << " = " << offset << "\n";
                }
                // dout << " (we start with offsets " << offset << " to cls " << ident->cls->name << " / " << layoutOffset
                //     << " cls-to-layout)\n";
                auto funcdef = ident->cls->getDefinition(functionName, offset, ident, ident->layout_cls, layoutOffset);
                auto func = funcdef.first;
                if (func) {
                  // Hack to handle strange destructors
                  if (functionName.equals("D1v") && func->hasName() && func->getName().endswith("D2Ev")) {
                    std::string newName = func->getName();
                    newName[newName.size() - 3] = '1';
                    auto func2 = M.getFunction(newName);
                    if (func2) {
                      dout << "Destructor hack: replace " << func->getName() << " with " << func2->getName() << "\n";
                      func = func2;
                    }
                  }
                  auto bb = getCached(funcdef);
                  if (bb) return bb;
                  bb = setCached(funcdef, BasicBlock::Create(C, getCaseName(ident), function));
                  IRBuilder<> builder(bb);
                  // cast arguments to correct type, modify "this" if necessary
                  std::vector<Value *> args;
                  auto defFuncType = func->getFunctionType();
                  for (size_t i = 0; i < function->arg_size(); i++) {
                    auto arg = function->arg_begin();
                    std::advance(arg, i);
                    if (funcdef.second != 0 && i == (size_t) thisArgNumber) {
                      dout << "Generating thunk " << funcdef.second << " for " << function->getName() << "\n";
                      auto val = builder.CreateBitOrPointerCast(arg, Int8Ptr());
                      val = builder.CreateConstGEP1_32(val, funcdef.second);
                      args.emplace_back(createCast(builder, val, defFuncType->getFunctionParamType(i)));
                    } else if (arg->getType() == defFuncType->getFunctionParamType(i)) {
                      args.emplace_back(arg);
                    } else {
                      args.emplace_back(createCast(builder, arg, defFuncType->getFunctionParamType(i)));
                    }
                  }
                  // Create call
                  if (func->getFunctionType()->getNumParams() != args.size()) {
                    dout << "Mismatch in argument types: " << func->getFunctionType()->getNumParams() << " != "
                         << args.size() << "\n";
                    dout << "Ident " << *ident << "\n";
                    dout << "func->type      = " << *func->getFunctionType() << "\n";
                    dout << "dispatcher type = " << *function->getFunctionType() << "\n";
                    dout << "disp name = " << function->getName() << "\n";
                    dout << "func name = " << func->getName() << "\n";
                  }
                  auto call = builder.CreateCall(func->getFunctionType(), func, args);
                  call->setCallingConv(func->getCallingConv());
                  // copy over parameter attributes
                  for (size_t i = 0; i < function->arg_size(); i++) {
                    if (function->hasParamAttribute(i, Attribute::AttrKind::ByVal))
                      call->addParamAttr(i, Attribute::AttrKind::ByVal);
                  }
                  // Set NOINLINE if necessary
                  if (BUILD_NOINLINE) {
                    func->addFnAttr(Attribute::NoInline);
                  }
                  // cast return if necessary
                  if (function->getReturnType()->isVoidTy()) {
                    builder.CreateRetVoid();
                  } else if (function->getReturnType() != call->getType()) {
                    /*
                    llvm::errs() << "[Warning] Return type does not match:\n";
                    llvm::errs() << " - " << function->getName() << " is " << *function->getType() << "\n";
                    llvm::errs() << " - " << func->getName() << " is " << *func->getType() << "\n";
                     */
                    builder.CreateRet(createCast(builder, call, function->getReturnType()));
                  } else {
                    builder.CreateRet(call);
                  }
                  return bb;
                }
                return parentBlock;
              }
          };
          DispatcherBuilder builder(&f);
          builder.thisArgNumber = thisArgNumber;
          builder.functionName = function;
          buildFunction(builder, getClass(classname));

          count_dispatchers++;
          count_dispatcher_usages += f.getNumUses();
        }

        void createBrokenFunction(Function &f) {
          f.addFnAttr(Attribute::NoInline);
          auto deadblock = BasicBlock::Create(C, "entry", &f);
          IRBuilder<> builder(deadblock);
          builder.CreateIntrinsic(Intrinsic::trap, {}, {});
          builder.CreateUnreachable();
        }

        void defineVirtualDispatcherPlaceholder(GlobalVariable &g) {
          std::string functionName = std::string("__novt.") + g.getName().substr(19).str();
          dout << "Define placeholder " << g.getName() << " as alias for " << functionName << "\n";
          auto func = M.getFunction(functionName);
          if (!func) {
            // Get class / function name of dispatcher
            auto idx = functionName.find_first_of('.', 7);
            if (idx == StringRef::npos) return;
            auto classname = functionName.substr(7, idx - 7);
            auto function = functionName.substr(idx + 1);
            // Find any call target to determine its type
            std::set<Function *> callTargets;
            collectCallTargets(callTargets, getClass(classname), function, 0);
            if (callTargets.empty()) {
              llvm::errs() << "[WARN] call target set for placeholder " << g.getName() << " is empty.\n";
              func = cast<Function>(
                      M.getOrInsertFunction(functionName, FunctionType::get(Type::getVoidTy(C), false)).getCallee());
              createBrokenFunction(*func);
            } else {
              auto Ty = (*callTargets.begin())->getFunctionType();
              // Create function and build
              func = cast<Function>(M.getOrInsertFunction(functionName, Ty).getCallee());
              defineVirtualDispatcher(*func);
              dout << " => defined with Type " << *Ty << "\n";
            }
          }
          auto replacementResult = typesafeReplaceAllUsesWith(&g, func);
          assert(replacementResult && "Could not replace dispatcher placeholder");
          dout << " => Survived replacement.\n";
        }

        void defineRttiGetter(Function &f) {
          auto classname = f.getName().substr(12);
          // llvm::errs() << "define: " << f.getName() << " class " << classname << "\n";
          assert(classes.count(classname) > 0);

          struct RTTIBuilder : public FunctionBuilder<DefClass *> {
              explicit RTTIBuilder(Function *function) : FunctionBuilder(RTTI_GET, function) {
                if (!BUILD_INSTRUMENTATION) {
                  function->addFnAttr(Attribute::AttrKind::NoFree);
                  function->addFnAttr(Attribute::AttrKind::ReadOnly);
                  function->addFnAttr(Attribute::AttrKind::NoUnwind);
                  function->addFnAttr(Attribute::AttrKind::WillReturn);
                }
              }

              BasicBlock *createCaseBlock(DefClassIdent *ident, bool isRoot, BasicBlock *parentBlock) {
                auto bb = getCached(ident->cls);
                if (bb) return bb;
                bb = setCached(ident->cls, BasicBlock::Create(C, getCaseName(ident), function));
                IRBuilder<> builder(bb);
                auto vtable = M.getNamedGlobal(ident->cls->vtableSymbol);
                // TODO that index is not always correct
                auto ptrToRttiPtr = builder.CreateInBoundsGEP(vtable, {
                        ConstantInt::get(Int32(), 0), ConstantInt::get(Int32(), 0), ConstantInt::get(Int32(), 1)});
                auto rttiPtr = builder.CreateLoad(ptrToRttiPtr);
                builder.CreateRet(rttiPtr);
                return bb;
              }
          };
          RTTIBuilder builder(&f);
          buildFunction(builder, getClass(classname));

          count_rtti++;
        }

        void defineDynamicCast(Function &f) {
          auto classNames = f.getName().substr(20);
          auto classNamesSplit = classNames.find('.');
          auto srcClass = getClass(classNames.substr(0, classNamesSplit));
          auto dstClass = getClass(classNames.substr(classNamesSplit + 1));
          assert(srcClass != nullptr && "srcClass is null");
          assert(dstClass != nullptr && "dstClass is null");

          struct DynamicCastBuilder : public FunctionBuilder<int> {
              DefClass *dst;

              explicit DynamicCastBuilder(Function *function, DefClass *dst)
                      : FunctionBuilder(DYNAMIC_CAST, function), dst(dst) {
                defaultIsImportant = true;
                if (!BUILD_INSTRUMENTATION) {
                  function->addFnAttr(Attribute::AttrKind::NoFree);
                  function->addFnAttr(Attribute::AttrKind::ReadOnly);
                  function->addFnAttr(Attribute::AttrKind::NoUnwind);
                  function->addFnAttr(Attribute::AttrKind::WillReturn);
                }
              }

              BasicBlock *createCaseBlock(DefClassIdent *ident, bool isRoot, BasicBlock *parentBlock) {
                llvm::Optional<int> offset;
                if (ident->cls == dst) {
                  // if dst is src, do the cast
                  offset = 0;
                } else {
                  // if dst is a base of src, do the cast
                  offset = ident->cls->getOffsetToBase(dst);
                }
                if (offset) {
                  auto address = function->arg_begin();
                  offset = *offset + ident->offsetToCls;
                  auto bb = getCached(*offset);
                  if (bb) return bb;
                  bb = setCached(*offset, BasicBlock::Create(C, getCaseName(ident), function));
                  IRBuilder<> builder(bb);
                  assert(address->getType() == Int8Ptr());
                  Value *adjustedAddress = (*offset != 0) ? builder.CreateConstGEP1_32(address, *offset) : address;
                  builder.CreateRet(builder.CreateBitCast(adjustedAddress, function->getReturnType()));
                  return bb;
                }
                return nullptr;
              }

              void buildDefaultCase(BasicBlock *defaultCaseBlock) {
                IRBuilder<> builder(defaultCaseBlock);
                builder.CreateRet(Constant::getNullValue(function->getReturnType()));
              }
          };
          DynamicCastBuilder builder(&f, dstClass);
          buildFunction(builder, srcClass);

          count_dynamic_cast++;
        }

        void defineDynamicCastToVoid(Function &f) {
          auto srcClass = getClass(f.getName().substr(25));
          assert(srcClass != nullptr && "srcClass is null");

          struct VoidCastBuilder : public FunctionBuilder<int> {
              explicit VoidCastBuilder(Function *function) : FunctionBuilder(DYNAMIC_VOID_CAST, function) {
                defaultIsImportant = true;
                if (!BUILD_INSTRUMENTATION) {
                  function->addFnAttr(Attribute::AttrKind::NoFree);
                  function->addFnAttr(Attribute::AttrKind::ReadOnly);
                  function->addFnAttr(Attribute::AttrKind::NoUnwind);
                  function->addFnAttr(Attribute::AttrKind::WillReturn);
                }
              }

              BasicBlock *createCaseBlock(DefClassIdent *ident, bool isRoot, BasicBlock *parentBlock) {
                auto bb = getCached(ident->offsetToCls);
                if (bb) return bb;
                bb = setCached(ident->offsetToCls, BasicBlock::Create(C, getCaseName(ident), function));
                IRBuilder<> builder(bb);
                auto address = function->arg_begin();
                Value *adjustedAddress = (ident->offsetToCls != 0)
                                         ? builder.CreateConstGEP1_32(address, ident->offsetToCls)
                                         : address;
                builder.CreateRet(builder.CreateBitCast(adjustedAddress, function->getReturnType()));
                return bb;
              }
          };
          VoidCastBuilder builder(&f);
          buildFunction(builder, srcClass);

          count_dynamic_cast_void++;
        }

        void defineVbaseOffsetGetter(Function &f) {
          // llvm::errs() << "define " << f.getName() << "\n";
          auto classNames = f.getName().substr(20);
          auto classNamesSplit = classNames.find('.');
          auto srcClass = getClass(classNames.substr(0, classNamesSplit));
          auto dstClass = getClass(classNames.substr(classNamesSplit + 1));
          assert(srcClass != nullptr && "srcClass is null");
          assert(dstClass != nullptr && "dstClass is null");

          struct VBaseOffsetBuilder : public FunctionBuilder<long> {
              DefClass *dst;

              explicit VBaseOffsetBuilder(Function *function, DefClass *dst)
                      : FunctionBuilder(VBASE_OFFSET, function), dst(dst) {
                if (!BUILD_INSTRUMENTATION) {
                  if (function->getFunctionType()->getParamType(0)->isIntegerTy()) {
                    function->addFnAttr(Attribute::AttrKind::ReadNone);
                  } else {
                    function->addFnAttr(Attribute::AttrKind::ReadOnly);
                  }
                  function->addFnAttr(Attribute::AttrKind::NoFree);
                  function->addFnAttr(Attribute::AttrKind::NoUnwind);
                  function->addFnAttr(Attribute::AttrKind::WillReturn);
                }
              }

              BasicBlock *createCaseBlock(DefClassIdent *ident, bool isRoot, BasicBlock *parentBlock) {
                llvm::errs() << function->getName() << ": use " << *ident << "\n";
                long offset;
                if (ident->layout_cls) {
                  auto baseOffset = ident->layout_cls->getOffsetToBase(dst);
                  assert(baseOffset && "did not find layout vbase");
                  offset = ident->offsetToLayoutCls + *baseOffset;
                } else {
                  auto baseOffset = ident->cls->getOffsetToBase(dst);
                  assert(baseOffset && "did not find vbase");
                  offset = ident->offsetToCls + *baseOffset;
                }
                auto bb = getCached(offset);
                if (bb) return bb;
                bb = setCached(offset, BasicBlock::Create(C, getCaseName(ident), function));
                IRBuilder<> builder(bb);
                builder.CreateRet(ConstantInt::get(cast<IntegerType>(function->getReturnType()), offset));
                return bb;
              }

              void buildDefaultCase(BasicBlock *defaultCaseBlock) {
                IRBuilder<> builder(defaultCaseBlock);
                if (BUILD_TRAPS && BUILD_TRAPS_VBASE_OFFSET) {
                  builder.CreateIntrinsic(Intrinsic::trap, {}, {});
                }
                builder.CreateUnreachable();
              }

              Value *buildIdentLoad(BasicBlock *block) {
                if (function->getFunctionType()->getParamType(0)->isIntegerTy())
                  return function->arg_begin();
                return FunctionBuilder::buildIdentLoad(block);
              }
          };
          VBaseOffsetBuilder builder(&f, dstClass);
          buildFunction(builder, srcClass);

          count_vbase_offset_getter++;
        }

        void printSummary() {
          llvm::errs() << "\n";
          llvm::errs() << count_dispatchers << " dispatchers generated\n";
          llvm::errs() << count_dispatcher_usages << " virtual calls\n";
          llvm::errs() << count_rtti << " rtti getter generated\n";
          llvm::errs() << count_dynamic_cast << " dynamic casters generated\n";
          llvm::errs() << count_dynamic_cast_void << " dynamic casters (void*) generated\n";
          llvm::errs() << count_vbase_offset_getter << " vbase offset getter generated\n";
        }

        void writeInstrumentationFile() {
          std::ofstream f("instrumentation-output.txt", std::ios::out | std::ios::trunc);
          for (auto &sf: createdFunctions) {
            const char *type_string;
            switch (sf->type) {
              case DISPATCHER:
                type_string = "dispatcher";
                break;
              case VBASE_OFFSET:
                type_string = "vbase";
                break;
              case DYNAMIC_CAST:
                type_string = "dynamic_cast";
                break;
              case DYNAMIC_VOID_CAST:
                type_string = "dynamic_cast_void";
                break;
              case RTTI_GET:
                type_string = "rtti";
                break;
            }
            f << type_string << "," << sf->function->getName().str() << "," << sf->cases.size() << ","
              << sf->blocks.size() << "\n";
          }

          /*auto main = M.getFunction("main");
          if (main) {
            for (auto& bb: main->getBasicBlockList()) {
              for (auto& ins: bb) {
                if (isa<ReturnInst>(ins)) {
                  IRBuilder<> builder(&bb);
                  builder.SetInsertPoint(&ins);
                  auto ft = FunctionType::get(Type::getVoidTy(C), {}, false);
                  builder.CreateCall(M.getOrInsertFunction("__instrument_print", ft), {});
                }
              }
            }
          }*/

          if (BUILD_INSTRUMENTATION == 2) {
            llvm::SMDiagnostic error;
            auto fname = "/usr/novt-lib/instrumentation/instrument.bc";
            auto M2 = llvm::parseIRFile(fname, error, C);
            llvm::Linker L(M);
            L.linkInModule(std::move(M2), Linker::LinkOnlyNeeded);
          }

          if (BUILD_INSTRUMENTATION == 3) {
            auto ft = FunctionType::get(Type::getVoidTy(C), {Int32, Int32}, false);
            for (auto & sf: createdFunctions) {
              //TODO link module
              IRBuilder<> builder(&sf->function->getEntryBlock().front());
              builder.CreateCall(M.getOrInsertFunction("__instrument_count_lite2", ft),
                                 {ConstantInt::get(Int32, sf->type, false), ConstantInt::get(Int32, sf->idents.size(), false)});
            }
          }
        }

        void printFunctionStatistics();

        void optDevirtualize();

        void optCollectIdentConnections();

        void optJoinEqualIdents();

        void optDeadIdentElimination();

        void optIdentNumbers();

        void optCleanMeta();

        bool run() {
          auto loadDuration = std::chrono::high_resolution_clock::now() - compilerStart;
          auto loadDurationSeconds = std::chrono::duration_cast<std::chrono::seconds>(loadDuration).count();
          auto start = std::chrono::high_resolution_clock::now();
          llvm::errs() << "[CONFIG] Building with trap instructions: " << (BUILD_TRAPS ? "yes" : "no") << "\n";
          llvm::errs() << "[CONFIG] Building without inlining trampolines: " << (BUILD_NOINLINE ? "yes" : "no") << "\n";
          llvm::errs() << "[CONFIG] Building without optimizing trampolines: "
                       << (BUILD_NOOPT_TRAMPOLINES ? "yes" : "no") << "\n";
          llvm::errs() << "[CONFIG] Building with optimizations: " << (BUILD_OPTIMIZE ? "yes" : "no") << "\n";
          llvm::errs() << "[CONFIG] Building with instrumentation: " << (BUILD_INSTRUMENTATION ? "yes" : "no") << "\n";
          dout << "Code load time: " << loadDurationSeconds << " seconds\n";

          buildClassHierarchy();
          dout << seconds_since(start) << "s  Class hierarchy created (" << classes.size() << ").\n";
          buildIdentHierarchy();
          dout << seconds_since(start) << "s  Ident hierarchy created (" << idents.size() << ").\n";
          buildClassNumbers();
          dout << seconds_since(start) << "s  class numbers created.\n";
          if (with_debug_output) {
            printClasses();
            printIdents();
          }

          for (auto &f: M.functions()) {
            if (f.isDeclaration()) {
              if (f.getName().startswith_lower("__novt.")) {
                defineVirtualDispatcher(f);
              } else if (f.getName().startswith_lower("__novt_rtti.")) {
                defineRttiGetter(f);
              } else if (f.getName().startswith_lower("__novt_dynamic_cast.")) {
                defineDynamicCast(f);
              } else if (f.getName().startswith_lower("__novt_dynamic_cast_void.")) {
                defineDynamicCastToVoid(f);
              } else if (f.getName().startswith_lower("__novt_vbase_offset.")) {
                defineVbaseOffsetGetter(f);
              }
            }
          }
          dout << seconds_since(start) << "s  functions defined.\n";
          for (auto &g: M.globals()) {
            if (g.getName().startswith_lower("__novt_placeholder.")) {
              defineVirtualDispatcherPlaceholder(g);
            }
          }
          dout << seconds_since(start) << "s  placeholders replaced.\n";

          auto startOpt = std::chrono::high_resolution_clock::now();
          if (BUILD_OPTIMIZE) {
            optCollectIdentConnections();
            optJoinEqualIdents();
            optDevirtualize();
            optDeadIdentElimination();

            // TODO test if this changes anything
            /*optCleanMeta();
            optCollectIdentConnections();
            optJoinEqualIdents();
            optDeadIdentElimination();
            optDevirtualize();*/

            optIdentNumbers();
          }
          dout << seconds_since(start) << "s  optimization finished.\n";
          auto elapsedOpt = std::chrono::high_resolution_clock::now() - startOpt;
          long long millisecondsOpt = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedOpt).count();
          patchConstructors();
          buildCases();
          printSummary();
          printFunctionStatistics();
          if (BUILD_INSTRUMENTATION) {
            writeInstrumentationFile();
          }

          auto elapsed = std::chrono::high_resolution_clock::now() - start;
          long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
          llvm::errs() << "[Stats] ABI Builder took " << milliseconds << " ms to complete (" << millisecondsOpt
                       << " ms to optimize).\n";

          return !classes.empty();
        }
    };

    void AbiBuilder::printFunctionStatistics() {
      long total_functions = 0;
      long total_blocks = 0;
      long total_cases = 0;
      long count_less_than_4_cases = 0;
      long count_less_than_4_blocks = 0;
      long count_empty_cases = 0;
      size_t max_cases = 0;
      size_t max_blocks = 0;
      for (auto &sf: createdFunctions) {
        if (sf->removed) continue;
        total_functions++;
        total_blocks += sf->blocks.size();
        total_cases += sf->cases.size();
        max_cases = std::max(max_cases, sf->cases.size());
        max_blocks = std::max(max_blocks, sf->blocks.size());
        if (sf->blocks.size() < 4) count_less_than_4_blocks++;
        if (sf->cases.size() < 4) count_less_than_4_cases++;
        if (sf->cases.empty()) count_empty_cases++;
        /*if (sf->cases.empty()) {
          llvm::errs() << *sf->function << "\n\n";
        }*/
      }
      long count_all_idents = 0;
      long count_idents = 0;
      for (auto &ident: idents) {
        if (ident->abstract) continue;
        count_all_idents++;
        if (!ident->replacedBy && !ident->removed)
          count_idents++;
      }
      llvm::errs() << count_idents << " non-abstract idents, " << (count_all_idents - count_idents) << " removed\n";
      llvm::errs() << total_functions << " functions, " << (createdFunctions.size() - total_functions) << " removed\n";
      llvm::errs() << total_blocks << " blocks (max " << max_blocks << ")\n";
      llvm::errs() << total_cases << " cases (max " << max_cases << ")\n";
      llvm::errs() << count_less_than_4_blocks << " functions with less than 4 blocks\n";
      llvm::errs() << count_less_than_4_cases << " functions with less than 4 cases\n";
      llvm::errs() << count_empty_cases << " functions with no cases at all\n";
    }

    /**
     * Remove all functions that have an unique call target / basic block
     */
    void AbiBuilder::optDevirtualize() {
      long count = 0;
      for (auto &sf: createdFunctions) {
        if (sf->removed) continue;
        if (!sf->defaultIsImportant && sf->blocks.size() == 1) {
          sf->replaceSwitchInst(*sf->blocks.begin());
          count++;
        } else if (sf->blocks.empty()) {
          sf->replaceSwitchInst(sf->switchInst->getDefaultDest());
          count++;
        }
      }
      llvm::errs() << "[OPT] Devirtualize removed " << count << " functions\n";
    }

    void AbiBuilder::optCollectIdentConnections() {
      for (auto &sf: createdFunctions) {
        for (auto c: sf->cases) {
          if (sf->defaultIsImportant) {
            for (auto &ident2: idents) {
              if (c.first != ident2.get()) {
                auto it = sf->cases.find(ident2.get());
                if (it != sf->cases.end() && c.second == it->second) {
                  if (c.first->equalCandidates.insert(ident2.get()).second) {
                    /*llvm::errs() << "equal coming from " << sf->function->getName() << ": " << c.first->number << " == "
                                 << ident2->number << " // " << *c.first << " == "
                                 << *ident2 << " (with default)\n";*/
                  }
                } else {
                  c.first->unequal.insert(ident2.get());
                  ident2->unequal.insert(c.first);
                  // llvm::errs() << c.first->number << " != " << ident2->number << "  // because " << sf->function->getName() << "\n";
                }
              }
            }
          } else {
            for (auto c2: sf->cases) {
              if (c.first != c2.first) {
                if (c.second == c2.second) {
                  c2.first->equalCandidates.insert(c.first);
                  if (c.first->equalCandidates.insert(c2.first).second) {
                    /*llvm::errs() << "equal coming from " << sf->function->getName() << ": " << c.first->number << " == " << c2.first->number << " // " << *c.first << " == " << *c2.first << "\n";*/
                  }
                } else {
                  c.first->unequal.insert(c2.first);
                  c2.first->unequal.insert(c.first);
                  // llvm::errs() << c.first->number << " != " << c2.first->number << "\n";
                }
              }
            }
          }
        }
      }
    }

    void AbiBuilder::optJoinEqualIdents() {
      long count = 0;
      for (auto &ident: idents) {
        if (ident->abstract) continue;
        DefClassIdent *ident1 = ident.get();
        while (ident1->replacedBy) ident1 = ident1->replacedBy;
        for (auto identIt2: ident->equalCandidates) {
          DefClassIdent *ident2 = identIt2;
          while (ident2->replacedBy) ident2 = ident2->replacedBy;
          if (ident.get() == ident2 || ident1 == ident2 || ident.get() == identIt2 || ident1 == identIt2) continue;
          if (ident->unequal.find(ident2) == ident->unequal.end() &&
              ident1->unequal.find(ident2) == ident1->unequal.end() &&
              ident->unequal.find(identIt2) == ident->unequal.end() &&
              ident1->unequal.find(identIt2) == ident1->unequal.end()) {
            // ident/ident1 and ident2 can be considered equal, nothing prevents this
            ident2->optReplaceBy(ident1);
            // llvm::errs() << "equal: " << ident1->number << " == " << ident2->number << " // " << *ident1 << "  ==  " << *ident2 << "\n";
            count++;
          }
        }
      }
      // Remove cases of removed idents
      for (auto &sf: createdFunctions) {
        std::set<DefClassIdent *> new_idents;
        for (auto identIt = sf->idents.cbegin(); identIt != sf->idents.cend();) {
          auto ident = *identIt;
          if (ident->replacedBy) {
            DefClassIdent *ident2 = ident->replacedBy;
            while (ident2->replacedBy) ident2 = ident2->replacedBy;

            // remove old case from structure
            auto it = sf->cases.find(ident);
            assert(it != sf->cases.end());
            auto bb = it->second;

            // remove / replace old case from switch inst
            sf->cases.erase(it);
            it = sf->cases.find(ident2);

            if (it == sf->cases.end()) {
              // "equal" case was not present here before
              sf->idents.insert(ident2);
              new_idents.insert(ident2);
            }
            sf->cases[ident2] = bb;
            identIt = sf->idents.erase(identIt);
          } else {
            ++identIt;
          }
        }
        for (auto ident: new_idents)
          sf->idents.insert(ident);
      }
      llvm::errs() << "[OPT] JoinEqualIdents combined " << count << " identifiers" << "\n";
    }

    void AbiBuilder::optDeadIdentElimination() {
      // Build set of removal candidates
      std::set<DefClassIdent *> unseenIdents;
      for (auto &ident: idents) {
        if (!ident->removed && !ident->replacedBy)
          unseenIdents.insert(ident.get());
      }
      // Exclude if ident is still necessary
      for (auto &sf: createdFunctions) {
        for (auto ident: sf->idents) {
          unseenIdents.erase(ident);
        }
        for (auto ident: sf->defaultIdents) {
          unseenIdents.erase(ident);
        }
      }
      // Remove dead idents
      for (auto ident: unseenIdents) {
        // dout << "Remove dead indent " << *ident << "\n";
        ident->remove();
      }
      llvm::errs() << "[OPT] DeadIdentElimination removed " << unseenIdents.size() << " idents\n";
    }

    /**
     * Helper function - build a "cluster" of idents that need different numbers
     * @param cluster
     * @param ident
     */
    void collectCluster(std::set<DefClassIdent *> &cluster, DefClassIdent *ident) {
      if (ident->in_cluster) return;
      cluster.insert(ident);
      ident->in_cluster = true;
      for (auto ident2: ident->unequal) {
        if (!ident2->removed && !ident2->replacedBy && !ident2->abstract && !ident2->in_cluster) {
          collectCluster(cluster, ident2);
        }
      }
    }

    uint32_t createClusterNumbers(std::vector<std::unique_ptr<SwitchFunction>> &createdFunctions,
                                  std::set<DefClassIdent *> &cluster, uint32_t min_number = 0) {
      if (cluster.size() > 5) {
        SwitchFunction *biggest = nullptr;  // the biggest set of idents that is: subset of cluster, not equal to cluster, size >4.
        for (auto &sf: createdFunctions) {
          if (sf->idents.size() <= 4 || sf->idents.size() >= cluster.size() ||
              (biggest && sf->idents.size() <= biggest->idents.size()))
            continue;
          bool allIdentsInCluster = true;
          for (auto id: sf->idents) {
            if (cluster.find(id) == cluster.end()) {
              allIdentsInCluster = false;
              break;
            }
          }
          if (allIdentsInCluster)
            biggest = sf.get();
        }

        // if we found a sub-cluster that is large enough, give it consecutive numbers
        if (biggest) {
          // llvm::errs() << "  - sub-cluster with " << biggest->idents.size() << " idents\n";
          min_number = createClusterNumbers(createdFunctions, biggest->idents, min_number);
          std::set<DefClassIdent *> remaining;
          std::set_difference(cluster.begin(), cluster.end(), biggest->idents.begin(), biggest->idents.end(),
                              std::inserter(remaining, remaining.end()));
          min_number = createClusterNumbers(createdFunctions, remaining, min_number);
          return min_number;
        }
      }

      if (cluster.size() > 4) {
        // keep previous ordering along class hierarchys if possible
        std::vector<DefClassIdent *> clusterSorted(cluster.begin(), cluster.end());
        std::sort(clusterSorted.begin(), clusterSorted.end(),
                  [](DefClassIdent *i1, DefClassIdent *i2) { return i1->number < i2->number; }
        );
        for (const auto &id: clusterSorted) {
          id->number = min_number++;
        }
      } else {
        // otherwise - screw it, doesn't matter.
        for (const auto &id: cluster) {
          id->number = min_number++;
        }
      }
      return min_number;
    }

    void AbiBuilder::optIdentNumbers() {
      uint32_t max_number_before = 0;
      uint32_t max_number = 0;

      for (const auto &ident: idents) {
        if (ident->removed || ident->replacedBy || ident->abstract) continue;
        max_number_before = std::max(max_number_before, ident->number);
      }

      for (const auto &ident: idents) {
        if (ident->removed || ident->replacedBy || ident->abstract || ident->in_cluster) continue;
        std::set<DefClassIdent *> cluster;
        collectCluster(cluster, ident.get());
        /* dout << "- cluster with size " << cluster.size() << "\n";
        for (auto clusterIdent: cluster) {
          dout << "  - " << *clusterIdent << "\n";
        } */
        auto num = createClusterNumbers(createdFunctions, cluster);
        max_number = std::max(max_number, num - 1);
      }

      llvm::errs() << "[OPT] All ident numbers are <= " << max_number << " (before: " << max_number_before << ")\n";
    }

    void AbiBuilder::optCleanMeta() {
      for (auto &ident: idents) {
        ident->unequal.clear();
        ident->equalCandidates.clear();
      }
    }

}

PreservedAnalyses NoVTAbiBuilderPass::run(Module &M, ModuleAnalysisManager &MAM) {
  llvm::errs() << "=== Build my C++ ABI ===\n";
  return AbiBuilder(M).run() ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool NoVTAbiBuilderLegacyPass::runOnModule(Module &M) {
  llvm::errs() << "=== Build my C++ ABI (legacy) ===\n";
  return AbiBuilder(M).run();
}

char NoVTAbiBuilderLegacyPass::ID = 0;

NoVTAbiBuilderLegacyPass::NoVTAbiBuilderLegacyPass() : ModulePass(ID) {}

void NoVTAbiBuilderLegacyPass::getAnalysisUsage(AnalysisUsage &AU) const {
}

static RegisterPass<NoVTAbiBuilderLegacyPass> Registration("NoVTAbiBuilder", "NoVTAbiBuilderLegacyPass", false, false);


char DebugLegacyPass::ID = 0;

bool DebugLegacyPass::runOnModule(Module &M) {
  auto start = std::chrono::high_resolution_clock::now();

  std::error_code code;
  raw_fd_ostream stream(fname, code);
  stream << M;
  stream.close();

  std::string name2 = fname + "-main.ll";
  raw_fd_ostream stream2(name2, code);
  if (M.getFunction("main")) {
    stream2 << *M.getFunction("main");
  } else {
    stream2 << "main not found\n";
  }
  stream2.close();

  dout << "Writing " << fname << " took " << seconds_since(start) << " seconds\n";

  return false;
}
