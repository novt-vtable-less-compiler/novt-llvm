#ifndef LLVM_NOVT_CUSTOM_CXX_ABI_H
#define LLVM_NOVT_CUSTOM_CXX_ABI_H

#define with_debug_output 0
#define dout if (with_debug_output) llvm::errs()
#define include_stdlib 1


class NoVTCustomCXXABI : public ItaniumCXXABI {
    std::string definedSuffix = "";
public:
    NoVTCustomCXXABI(CodeGen::CodeGenModule &CGM, bool UseARMMethodPtrABI = false, bool UseARMGuardVarABI = false)
            : ItaniumCXXABI(CGM, UseARMMethodPtrABI, UseARMGuardVarABI) {
      dout << "[DEBUG] Created NoVTCustomCXXABI with debug output\n";
    }

    CGCallee
    EmitLoadOfMemberFunctionPointer(CodeGenFunction &CGF,
                                    const Expr *E,
                                    Address This,
                                    llvm::Value *&ThisPtrForCall,
                                    llvm::Value *MemFnPtr,
                                    const MemberPointerType *MPT) override {
      // TODO in this area improvements might happen
      auto result = ItaniumCXXABI::EmitLoadOfMemberFunctionPointer(CGF, E, This, ThisPtrForCall, MemFnPtr, MPT);
      // llvm::errs() << "[DEBUG] EmitLoadOfMemberFunctionPointer " << *ThisPtrForCall << "  " << *MemFnPtr << "\n";
      return result;
    }

    llvm::Constant *EmitMemberFunctionPointer(const CXXMethodDecl *MD) override {
      assert(MD->isInstance() && "Member function must not be static!");
      if (MD->isVirtual() && !excludeFromAbi(MD)) {
        auto RD = MD->getParent();

        // get or create dispatch function
        std::string dispatcherName = "__novt." + mangleClass(RD) + "." + mangleFunctionNameForInheritance(MD);
        auto &M = (llvm::Module &) CGM.getModule();
        llvm::Value *dispatcher = M.getFunction(dispatcherName);
        if (!dispatcher) {
          dispatcherName = "__novt_placeholder." + mangleClass(RD) + "." + mangleFunctionNameForInheritance(MD);
          dispatcher = M.getOrInsertGlobal(dispatcherName, llvm::IntegerType::getInt8Ty(CGM.getLLVMContext()));
          M.getGlobalVariable(dispatcherName)->setLinkage(llvm::GlobalVariable::ExternalLinkage);
        }
        // llvm::errs() << *dispatcher << "\n\n";

        llvm::Constant *MemPtr[2];
        MemPtr[0] = llvm::ConstantExpr::getPtrToInt(cast<llvm::GlobalValue>(dispatcher),
                                                    CGM.PtrDiffTy); // llvm::ConstantInt::get(CGM.PtrDiffTy, dispatcher);
        MemPtr[1] = llvm::ConstantInt::get(CGM.PtrDiffTy, 0);
        return llvm::ConstantStruct::getAnon(MemPtr);
      }

      auto result = ItaniumCXXABI::EmitMemberFunctionPointer(MD);
      // llvm::errs() << "[DEBUG] EmitMemberFunctionPointer " << *result << "\n";
      return result;
    }

    llvm::Constant *getVTableAddressPoint(BaseSubobject Base,
                                          const CXXRecordDecl *VTableClass) override {
      auto result = ItaniumCXXABI::getVTableAddressPoint(Base, VTableClass);
      // llvm::errs() << "[DEBUG] getVTableAddressPoint " << *result << "\n";
      return result;
    }

    llvm::Value *GetVirtualBaseClassOffset(CodeGenFunction &CGF, Address This,
                                           const CXXRecordDecl *ClassDecl,
                                           const CXXRecordDecl *BaseClassDecl) override {
      if (excludeFromAbi(ClassDecl) && excludeFromAbi(BaseClassDecl)) {
        return ItaniumCXXABI::GetVirtualBaseClassOffset(CGF, This, ClassDecl, BaseClassDecl);
      }
      emitBasicTypeInformationToMeta(ClassDecl);
      emitBasicTypeInformationToMeta(BaseClassDecl);
      auto funcname = "__novt_vbase_offset." + mangleClass(ClassDecl) + "." + mangleClass(BaseClassDecl);
      /*
      llvm::FunctionType *functype = llvm::FunctionType::get(CGM.PtrDiffTy, {CGM.PtrDiffTy}, false);
      auto getOffsetFunction = CGM.getModule().getOrInsertFunction(funcname, functype);
      auto pointer = This.getPointer();
      if (pointer->getType() != CGM.PtrDiffTy->getPointerTo())
        pointer = CGF.Builder.CreateBitOrPointerCast(pointer, CGM.PtrDiffTy->getPointerTo());
      auto ident = CGF.Builder.CreateAlignedLoad(pointer, CGM.getModule().getDataLayout().getPointerPrefAlignment());
      return CGF.Builder.CreateCall(getOffsetFunction, {ident});
      // */
      //*
      llvm::FunctionType *functype = llvm::FunctionType::get(CGM.PtrDiffTy, {This.getType()}, false);
      auto getOffsetFunction = CGM.getModule().getOrInsertFunction(funcname, functype);
      return CGF.Builder.CreateCall(getOffsetFunction, {This.getPointer()});
      // */
    }

    llvm::Value *EmitTypeid(CodeGenFunction &CGF, QualType SrcRecordTy,
                            Address ThisPtr,
                            llvm::Type *StdTypeInfoPtrTy) override {
      auto &C = CGM.getLLVMContext();
      if (excludeFromAbi(SrcRecordTy->getAsCXXRecordDecl())) {
        return ItaniumCXXABI::EmitTypeid(CGF, SrcRecordTy, ThisPtr, StdTypeInfoPtrTy);
      }

      emitBasicTypeInformationToMeta(SrcRecordTy->getAsCXXRecordDecl());

      // getRTTIPointer = load class id, map to vtable[-1] / vtable[1], get a pointer back
      auto *ClassDecl = cast<CXXRecordDecl>(SrcRecordTy->getAs<RecordType>()->getDecl());
      auto funcname = "__novt_rtti." + mangleClass(ClassDecl);
      llvm::FunctionType *functype = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(C), {ThisPtr.getType()}, false);
      auto getRttiFunction = CGM.getModule().getOrInsertFunction(funcname, functype);
      //llvm::Value* rttiPtr = CGF.Builder.CreateCall(getRttiFunction, llvm::ArrayRef<llvm::Value*>({ThisPtr}));
      llvm::Value *rttiPtr = CGF.Builder.CreateCall(getRttiFunction, {ThisPtr.getPointer()});
      auto result = CGF.Builder.CreateBitCast(rttiPtr, StdTypeInfoPtrTy);

      // auto result = ItaniumCXXABI::EmitTypeid(CGF, SrcRecordTy, ThisPtr, StdTypeInfoPtrTy);
      // llvm::errs() << "[DEBUG] EmitTypeid " << *result << "\n";
      return result;
    }

    llvm::Value *EmitDynamicCastCall(CodeGenFunction &CGF, Address ThisAddr,
                                     QualType SrcRecordTy, QualType DestTy,
                                     QualType DestRecordTy,
                                     llvm::BasicBlock *CastEnd) override {

      if (excludeFromAbi(SrcRecordTy->getAsCXXRecordDecl()) && excludeFromAbi(DestRecordTy->getAsCXXRecordDecl())) {
        auto result = ItaniumCXXABI::EmitDynamicCastCall(CGF, ThisAddr, SrcRecordTy, DestTy, DestRecordTy, CastEnd);
        dout << "[DEBUG] EmitDynamicCastCall on std:: " << *result << "\n";
        return result;
      }

      emitBasicTypeInformationToMeta(SrcRecordTy->getAsCXXRecordDecl());
      emitBasicTypeInformationToMeta(DestRecordTy->getAsCXXRecordDecl());

      // call dynamic_cast
      std::string dispatcherName = "__novt_dynamic_cast." + mangleClass(SrcRecordTy->getAsCXXRecordDecl())
                                   + "." + mangleClass(DestRecordTy->getAsCXXRecordDecl());
      auto &M = (llvm::Module &) CGF.CGM.getModule();
      auto ptr = llvm::Type::getInt8PtrTy(M.getContext());
      auto dispatcher = M.getOrInsertFunction(dispatcherName, llvm::FunctionType::get(ptr, {ptr}, false)).getCallee();
      llvm::Value *Value = ThisAddr.getPointer();
      Value = CGF.Builder.CreateBitCast(Value, ptr);
      Value = CGF.Builder.CreateCall(dispatcher, {Value});
      Value = CGF.Builder.CreateBitCast(Value, CGF.ConvertType(DestTy));

      /// C++ [expr.dynamic.cast]p9:
      ///   A failed cast to reference type throws std::bad_cast
      if (DestTy->isReferenceType()) {
        llvm::BasicBlock *BadCastBlock =
                CGF.createBasicBlock("dynamic_cast.bad_cast");

        llvm::Value *IsNull = CGF.Builder.CreateIsNull(Value);
        CGF.Builder.CreateCondBr(IsNull, BadCastBlock, CastEnd);

        CGF.EmitBlock(BadCastBlock);
        EmitBadCastCall(CGF);
      }

      return Value;
    }

    llvm::Value *EmitDynamicCastToVoid(CodeGenFunction &CGF, Address Value,
                                       QualType SrcRecordTy,
                                       QualType DestTy) override {
      if (excludeFromAbi(SrcRecordTy->getAsCXXRecordDecl())) {
        auto result = ItaniumCXXABI::EmitDynamicCastToVoid(CGF, Value, SrcRecordTy, DestTy);
        dout << "[DEBUG] EmitDynamicCastToVoid " << *result << "\n";
        return result;
      }

      emitBasicTypeInformationToMeta(SrcRecordTy->getAsCXXRecordDecl());

      // call dynamic_cast
      std::string dispatcherName = "__novt_dynamic_cast_void." + mangleClass(SrcRecordTy->getAsCXXRecordDecl());
      auto &M = (llvm::Module &) CGF.CGM.getModule();
      auto ptr = llvm::Type::getInt8PtrTy(M.getContext());
      auto dispatcher = M.getOrInsertFunction(dispatcherName, llvm::FunctionType::get(ptr, {ptr}, false)).getCallee();
      llvm::Value *Result = Value.getPointer();
      Result = CGF.Builder.CreateBitCast(Result, ptr);
      Result = CGF.Builder.CreateCall(dispatcher, {Result});
      Result = CGF.Builder.CreateBitCast(Result, CGF.ConvertType(DestTy));

      return Result;
    }

    CGCallee getVirtualFunctionPointer(CodeGenFunction &CGF, GlobalDecl GD,
                                       Address This, llvm::Type *Ty,
                                       SourceLocation Loc) override {
      dout << "[DEBUG] getVirtualFunctionPointer (type " << *Ty << ")\n";

      // check if that's a std:: class
      auto *MethodDecl = cast<CXXMethodDecl>(GD.getDecl());
      if (excludeFromAbi(MethodDecl)) {
        llvm::errs() << "        => Itanium because method " << mangle(MethodDecl) << " is in std namespace: "
                     << MethodDecl->isStdNamespace() << " " << MethodDecl->isInStdNamespace() << " "
                     << MethodDecl->getEnclosingNamespaceContext()->isStdNamespace() << "\n";
        return ItaniumCXXABI::getVirtualFunctionPointer(CGF, GD, This, Ty, Loc);
      }
      /* if (Ty->isFunctionTy() && Ty->getFunctionNumParams() > 0 && Ty->getFunctionParamType(0)->isPointerTy()) {
        auto classType = Ty->getFunctionParamType(0)->getPointerElementType();
        if (classType->isStructTy() && classType->getStructName().startswith("class.std::")) {
          llvm::errs() << "        => Itanium\n";
          return ItaniumCXXABI::getVirtualFunctionPointer(CGF, GD, This, Ty, Loc);
        }
      } */

      // get or create dispatch function
      std::string dispatcherName = "__novt." + mangleClass(MethodDecl->getParent())
                                   + "." + mangleFunctionNameForInheritance(GD);
      auto &M = (llvm::Module &) CGF.CGM.getModule();
      auto dispatcher = M.getOrInsertFunction(dispatcherName, cast<llvm::FunctionType>(Ty)).getCallee();

      // llvm::errs() << "call " << MethodDecl->getParent()->getName() << "::" << MethodDecl->getName() << "  overridden="
      //             << MethodDecl->size_overridden_methods() << "\n";

      CGCallee Callee(GD, dispatcher);
      return Callee;
      // return ItaniumCXXABI::getVirtualFunctionPointer(CGF, GD, This, Ty, Loc);
    }

    llvm::Value *EmitVirtualDestructorCall(CodeGenFunction &CGF,
                                           const CXXDestructorDecl *Dtor,
                                           CXXDtorType DtorType, Address This,
                                           DeleteOrMemberCallExpr E) override {
      auto result = ItaniumCXXABI::EmitVirtualDestructorCall(CGF, Dtor, DtorType, This, E);
      if (result) {
        dout << "[DEBUG] EmitVirtualDestructorCall " << *result << "\n";
      } else {
        dout << "[DEBUG] EmitVirtualDestructorCall <nullptr> for " << Dtor->getNameAsString() << " " << DtorType
             << "\n";
      }
      return result;
    }

    inline bool excludeFromAbi(const CXXMethodDecl *method) {
      if (include_stdlib) return false;
      return method->isStdNamespace() || method->isInStdNamespace() ||
             method->getEnclosingNamespaceContext()->isStdNamespace();
    }

    inline bool excludeFromAbi(const CXXRecordDecl *rd) {
      if (rd == nullptr) return true;
      if (include_stdlib) return false;
      return rd->isStdNamespace() || rd->isInStdNamespace() || rd->getEnclosingNamespaceContext()->isStdNamespace();
    }

    inline std::string mangle(const NamedDecl *decl, CXXDtorType dtor = CXXDtorType::Dtor_Base) {
      std::string name;
      llvm::raw_string_ostream nameStream(name);
      auto destructor = dyn_cast<CXXDestructorDecl>(decl);
      if (destructor) {
        getMangleContext().mangleCXXDtor(destructor, dtor, nameStream);
      } else {
        getMangleContext().mangleCXXName(decl, nameStream);
      }
      return nameStream.str();
    }

    inline std::string mangle(const GlobalDecl *decl) {
      std::string name;
      llvm::raw_string_ostream nameStream(name);
      auto destructor = dyn_cast<CXXDestructorDecl>(decl->getDecl());
      auto namedDecl = dyn_cast<NamedDecl>(decl->getDecl());
      if (destructor) {
        getMangleContext().mangleCXXDtor(destructor, decl->getDtorType(), nameStream);
      } else {
        assert(namedDecl);
        getMangleContext().mangleCXXName(namedDecl, nameStream);
      }
      return nameStream.str();
    }

    inline std::string
    mangleFunctionNameForInheritance(const FunctionDecl *decl, CXXDtorType dtor = CXXDtorType::Dtor_Base) {
      std::string name;
      llvm::raw_string_ostream nameStream(name);
      getMangleContext().mangleFunctionNameForInheritance(nameStream, decl, dtor);
      return nameStream.str();
    }

    inline std::string mangleFunctionNameForInheritance(const GlobalDecl &decl) {
      std::string name;
      llvm::raw_string_ostream nameStream(name);
      auto methodDecl = dyn_cast<CXXMethodDecl>(decl.getDecl());
      auto dtor = isa<CXXDestructorDecl>(decl.getDecl()) ? decl.getDtorType() : CXXDtorType::Dtor_Base;
      getMangleContext().mangleFunctionNameForInheritance(nameStream, methodDecl, dtor);
      return nameStream.str();
    }

    inline std::string mangleThunk(const CXXMethodDecl *decl, const ThunkInfo &info, CXXDtorType dtor = Dtor_Base) {
      std::string name;
      llvm::raw_string_ostream nameStream(name);
      auto destructor = dyn_cast<CXXDestructorDecl>(decl);
      if (destructor) {
        getMangleContext().mangleCXXDtorThunk(destructor, dtor, info.This, nameStream);
      } else {
        getMangleContext().mangleThunk(decl, info, nameStream);
      }
      return nameStream.str();
    }

    inline std::string &getDefinedSuffix() {
      if (definedSuffix.empty()) {
        definedSuffix = "__defined_in__" + CGM.getModule().getSourceFileName();
        for (auto &it: definedSuffix) {
          if (it != '-' && !isalnum(it)) {
            it = '_';
          }
        }
      }
      return definedSuffix;
    }

    inline std::string mangleClass(const CXXRecordDecl *decl) {
      std::string name;
      llvm::raw_string_ostream nameStream(name);
      // getMangleContext().mangleNameOrStandardSubstitution(decl);
      getMangleContext().mangleCXXVTable(decl, nameStream);
      std::string result = nameStream.str().substr(4);
      if (decl->isInAnonymousNamespace()) {
        result += getDefinedSuffix();
      }
      return result;
    }

    llvm::MDTuple *getMetaForMethod(CXXMethodDecl *method, CXXDtorType dtor, CXXDtorType dtor2) {
      auto &C = CGM.getLLVMContext();
      auto symbol = mangle(method, dtor2);
      std::vector<llvm::Metadata *> data{llvm::MDString::get(C, mangleFunctionNameForInheritance(method, dtor)),
                                         llvm::MDString::get(C, symbol)};
      auto func = CGM.getModule().getFunction(symbol);
      if (func) {
        data.push_back(llvm::ValueAsMetadata::get(func));
      } else {
        data.push_back(llvm::MDString::get(C, ""));
      }

      // Add info about defined thunks
      auto thunks = CGM.getItaniumVTableContext().getThunkInfo(
              isa<CXXDestructorDecl>(method) ? GlobalDecl(cast<CXXDestructorDecl>(method), dtor) : method
      );
      if (thunks) {
        for (auto thunk: *thunks) {
          if (thunk.This.Virtual.Itanium.VCallOffsetOffset)
            continue;
          auto thunkname = mangleThunk(method, thunk, dtor);
          auto thunkSym = CGM.getModule().getFunction(thunkname);
          if (thunkSym) {
            data.push_back(llvm::MDString::get(C, std::to_string(thunk.This.NonVirtual)));
            data.push_back(llvm::ValueAsMetadata::get(thunkSym));
          }
        }
      }
      // Save
      return llvm::MDNode::get(C, data);
    }

    void emitVTableDefinitions(CodeGenVTables &CGVT, const CXXRecordDecl *RD) override {
      // llvm::errs() << "emitVTableDefinitions " << RD->getName() << " = " << mangleClass(RD) << "\n";
      ItaniumCXXABI::emitVTableDefinitions(CGVT, RD);
      emitTypeInformationToMeta(RD, true);
    }

    void emitTypeInformationToMeta(const CXXRecordDecl *RD, bool mustEmitVtable) {
      // ignore standard declarations
      if (excludeFromAbi(RD))
        return;

      // We record:
      // Class => [parent classes]
      // Class => [virtual Methods]

      auto &C = CGM.getLLVMContext();
      auto classname = mangleClass(RD);
      if (CGM.getModule().getNamedMetadata("novt." + classname + ".vtable"))
        return;
      // llvm::errs() << "Class " << RD->getQualifiedNameAsString() << " = " << mangleClass(RD) << "\n";

      if (!RD->isAbstract() || mustEmitVtable) {
        // Real VTable
        auto vtableName = getAddrOfVTable(RD, CharUnits())->getName();
        auto metaVtable = CGM.getModule().getOrInsertNamedMetadata("novt." + classname + ".vtable");
        auto vtable = CGM.getModule().getNamedGlobal(vtableName);
        const VTableLayout &VTLayout = CGM.getItaniumVTableContext().getVTableLayout(RD);
        auto indexMap = llvm::MDString::get(C, computeVTLayoutMap(&VTLayout));
        if (vtable) {
          metaVtable->addOperand(llvm::MDNode::get(C, {llvm::ValueAsMetadata::get(vtable), indexMap}));
        } else {
          metaVtable->addOperand(llvm::MDNode::get(C, {llvm::MDString::get(C, vtableName), indexMap}));
        }
      }

      // Construction VTable
      auto cvtables = CGM.getModule().getNamedMetadata("novt." + classname + ".cvtables");
      if (cvtables) {
        for (unsigned i = 0; i < cvtables->getNumOperands(); i++) {
          if (cvtables->getOperand(i)->getNumOperands() == 5) {
            auto symbol = cast<llvm::MDString>(cvtables->getOperand(i)->getOperand(0))->getString();
            auto table = CGM.getModule().getNamedGlobal(symbol);
            if (table) {
              cvtables->setOperand(i, llvm::MDNode::get(C, {
                      cvtables->getOperand(i)->getOperand(0),
                      cvtables->getOperand(i)->getOperand(1),
                      cvtables->getOperand(i)->getOperand(2),
                      cvtables->getOperand(i)->getOperand(3),
                      cvtables->getOperand(i)->getOperand(4),
                      llvm::ValueAsMetadata::get(table)
              }));
            }
          }
        }
      }

      emitBasicTypeInformationToMeta(RD);

      for (const CXXBaseSpecifier &base: RD->bases()) {
        const CXXRecordDecl *baseDecl = cast<CXXRecordDecl>(base.getType()->getAs<RecordType>()->getDecl());
        if (baseDecl && !base.isVirtual() && baseDecl->isAbstract()) {
          emitTypeInformationToMeta(baseDecl, false);
        }
      }
    }

    void emitBasicTypeInformationToMeta(const CXXRecordDecl *RD) {
      if (RD == nullptr) {
        llvm::errs() << "[WARN] emitBasicTypeInformationToMeta(nullptr)\n";
        return;
      }
      auto &C = CGM.getLLVMContext();
      auto classname = mangleClass(RD);
      if (CGM.getModule().getNamedMetadata("novt." + classname + ".bases"))
        return;

      bool inheritsIgnored = false;
      bool hasVirtualMethods = false;

      auto metaBases = CGM.getModule().getOrInsertNamedMetadata("novt." + classname + ".bases");
      for (const CXXBaseSpecifier &base: RD->bases()) {
        const CXXRecordDecl *baseDecl = cast<CXXRecordDecl>(base.getType()->getAs<RecordType>()->getDecl());
        if (baseDecl) {
          if (base.isVirtual())
            continue;
          const ASTRecordLayout &Layout = getContext().getASTRecordLayout(RD);
          // auto offset = base.isVirtual() ? Layout.getVBaseClassOffset(baseDecl) : Layout.getBaseClassOffset(baseDecl);
          auto offset = Layout.getBaseClassOffset(baseDecl);
          auto mangledClassName = mangleClass(baseDecl);
          auto N = llvm::MDNode::get(C, {llvm::MDString::get(C, mangledClassName),
                                         llvm::MDString::get(C, std::to_string(offset.getQuantity()))});
          metaBases->addOperand(N);
          if (excludeFromAbi(baseDecl)) inheritsIgnored = true;
          // llvm::errs() << classname << " has base " << baseDecl->getQualifiedNameAsString() << " ("
          //              << mangleClass(baseDecl) << ") with offset " << offset.getQuantity() << "\n";
        } else {
          llvm::errs() << "[WARNING]: unknown base\n";
        }
      }
      auto metaVBases = CGM.getModule().getOrInsertNamedMetadata("novt." + classname + ".vbases");
      for (const CXXBaseSpecifier &base: RD->vbases()) {
        const CXXRecordDecl *baseDecl = cast<CXXRecordDecl>(base.getType()->getAs<RecordType>()->getDecl());
        if (baseDecl) {
          const ASTRecordLayout &Layout = getContext().getASTRecordLayout(RD);
          auto offset = Layout.getVBaseClassOffset(baseDecl);
          auto N = llvm::MDNode::get(C, {llvm::MDString::get(C, mangleClass(baseDecl)),
                                         llvm::MDString::get(C, std::to_string(offset.getQuantity()))});
          metaVBases->addOperand(N);
          // llvm::errs() << classname << " has vbase " << mangleClass(baseDecl) << " with offset " << offset.getQuantity()
          //              << "\n";
        } else {
          llvm::errs() << "WARNING: unknown base\n";
        }
      }

      auto metaMethods = CGM.getModule().getOrInsertNamedMetadata("novt." + classname + ".methods");
      for (CXXMethodDecl *method: RD->methods()) {
        if (!method->isVirtual()) continue;
        hasVirtualMethods = true;

        /*if (method->getIdentifier()) {
          llvm::errs() << RD->getName() << "::" << method->getName() << " => " << mangle(method) << "\n";
        } else {
          llvm::errs() << "[no identifier] " << RD->getName() << "::" << *method << " => mangled ";
          llvm::errs() << mangle(method) << "\n";
        }*/
        auto destructor = dyn_cast<CXXDestructorDecl>(method);
        if (destructor) {
          metaMethods->addOperand(getMetaForMethod(method, Dtor_Base, Dtor_Base));
          metaMethods->addOperand(getMetaForMethod(method, Dtor_Deleting, Dtor_Deleting));
          metaMethods->addOperand(getMetaForMethod(method, Dtor_Complete, Dtor_Base));
          metaMethods->addOperand(getMetaForMethod(method, Dtor_Comdat, Dtor_Comdat));
        } else {
          metaMethods->addOperand(getMetaForMethod(method, Dtor_Base, Dtor_Base));
        }
      }

      for (const CXXBaseSpecifier &base: RD->bases()) {
        const CXXRecordDecl *baseDecl = cast<CXXRecordDecl>(base.getType()->getAs<RecordType>()->getDecl());
        emitBasicTypeInformationToMeta(baseDecl);
      }

      if (!excludeFromAbi(RD) && inheritsIgnored && hasVirtualMethods) {
        llvm::errs() << "[ERROR] Class has virtual methods AND inherits from stdlib: " << RD->getName() << " ("
                     << mangle(RD) << ")\n";
        llvm::report_fatal_error("Inheritance from stdlib", true);
      }
    }

    std::string computeVTLayoutMap(const VTableLayout *VTLayout) {
      // Map vtable index to offset in layout class
      std::map<int, int> vtableIndexToLayoutClassOffset;
      for (auto it: VTLayout->getAddressPoints()) {
        vtableIndexToLayoutClassOffset[it.second.VTableIndex] = it.first.getBaseOffset().getQuantity();
        // llvm::errs() << "[vtableIndexToLayoutClassOffset] for " << Base.getBase()->getName() << " in " << RD->getName() << "  |  " << it.first.getBase()->getName() << " says " << it.second.VTableIndex << " => offset " << it.first.getBaseOffset().getQuantity() << "\n";
      }

      // Serialize this map
      std::string indexMap = "";
      for (const auto &it: vtableIndexToLayoutClassOffset) {
        indexMap += std::to_string(it.first) + "," + std::to_string(it.second) + ";";
      }
      return indexMap;
    }

    void
    onCreateConstructionVTable(const llvm::GlobalVariable *VTable, const CXXRecordDecl *RD, const BaseSubobject &Base,
                               bool BaseIsVirtual, const VTableLayout *VTLayout) override {
      auto indexMap = computeVTLayoutMap(VTLayout);

      // Emit metadata node
      auto &C = CGM.getLLVMContext();
      auto metaCTors = CGM.getModule().getOrInsertNamedMetadata("novt." + mangleClass(RD) + ".cvtables");
      auto N = llvm::MDNode::get(C, {llvm::MDString::get(C, VTable->getName()),
                                     llvm::MDString::get(C, mangleClass(Base.getBase())),
                                     llvm::MDString::get(C, mangleClass(RD)),
                                     llvm::MDString::get(C, std::to_string(Base.getBaseOffset().getQuantity())),
                                     llvm::MDString::get(C, indexMap),
              //llvm::ValueAsMetadata::get((llvm::GlobalVariable*) VTable)
      });
      metaCTors->addOperand(N);
    }
};





class NoVTCustomARMCXXABI : public NoVTCustomCXXABI {
public:
  NoVTCustomARMCXXABI(CodeGen::CodeGenModule &CGM) :
    NoVTCustomCXXABI(CGM, /*UseARMMethodPtrABI=*/true, /*UseARMGuardVarABI=*/true) {}

  bool HasThisReturn(GlobalDecl GD) const override {
    return (isa<CXXConstructorDecl>(GD.getDecl()) || (
              isa<CXXDestructorDecl>(GD.getDecl()) &&
              GD.getDtorType() != Dtor_Deleting));
  }

  void EmitReturnFromThunk(CodeGenFunction &CGF, RValue RV,
                           QualType ResultType) override {
    if (!isa<CXXDestructorDecl>(CGF.CurGD.getDecl()))
    return ItaniumCXXABI::EmitReturnFromThunk(CGF, RV, ResultType);

    // Destructor thunks in the ARM ABI have indeterminate results.
    llvm::Type *T = CGF.ReturnValue.getElementType();
    RValue Undef = RValue::get(llvm::UndefValue::get(T));
    return ItaniumCXXABI::EmitReturnFromThunk(CGF, Undef, ResultType);
  }

  CharUnits getArrayCookieSizeImpl(QualType elementType) override {
    // ARM says that the cookie is always:
    //   struct array_cookie {
    //     std::size_t element_size; // element_size != 0
    //     std::size_t element_count;
    //   };
    // But the base ABI doesn't give anything an alignment greater than
    // 8, so we can dismiss this as typical ABI-author blindness to
    // actual language complexity and round up to the element alignment.
    return std::max(CharUnits::fromQuantity(2 * CGM.SizeSizeInBytes),
                    CGM.getContext().getTypeAlignInChars(elementType));
  }

  Address InitializeArrayCookie(CodeGenFunction &CGF,
                                Address newPtr,
                                llvm::Value *numElements,
                                const CXXNewExpr *expr,
                                QualType elementType) override {
    assert(requiresArrayCookie(expr));

    // The cookie is always at the start of the buffer.
    Address cookie = newPtr;

    // The first element is the element size.
    cookie = CGF.Builder.CreateElementBitCast(cookie, CGF.SizeTy);
    llvm::Value *elementSize = llvm::ConstantInt::get(CGF.SizeTy,
                   getContext().getTypeSizeInChars(elementType).getQuantity());
    CGF.Builder.CreateStore(elementSize, cookie);

    // The second element is the element count.
    cookie = CGF.Builder.CreateConstInBoundsGEP(cookie, 1);
    CGF.Builder.CreateStore(numElements, cookie);

    // Finally, compute a pointer to the actual data buffer by skipping
    // over the cookie completely.
    CharUnits cookieSize = getArrayCookieSizeImpl(elementType);
    return CGF.Builder.CreateConstInBoundsByteGEP(newPtr, cookieSize);
  }

  llvm::Value *readArrayCookieImpl(CodeGenFunction &CGF, Address allocPtr,
                                   CharUnits cookieSize) override {
    // The number of elements is at offset sizeof(size_t) relative to
    // the allocated pointer.
    Address numElementsPtr
      = CGF.Builder.CreateConstInBoundsByteGEP(allocPtr, CGF.getSizeSize());

    numElementsPtr = CGF.Builder.CreateElementBitCast(numElementsPtr, CGF.SizeTy);
    return CGF.Builder.CreateLoad(numElementsPtr);
  }
};

#endif //LLVM_NOVT_CUSTOM_CXX_ABI_H
