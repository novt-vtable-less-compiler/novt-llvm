#ifndef LLVM_NoVTAbiBuilder_H
#define LLVM_NoVTAbiBuilder_H

#include <llvm/IR/PassManager.h>

#include <utility>

namespace llvm {

    struct NoVTAbiBuilderPass : public PassInfoMixin<NoVTAbiBuilderPass> {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
    };

    struct NoVTAbiBuilderLegacyPass : public ModulePass {
        static char ID;

        NoVTAbiBuilderLegacyPass();

        bool runOnModule(Module &M) override;

        void getAnalysisUsage(AnalysisUsage &usage) const override;

    };

    struct DebugLegacyPass : public ModulePass {
        static char ID;
        std::string fname;

        inline explicit DebugLegacyPass(std::string fname) : ModulePass(ID), fname(std::move(fname)){}

        bool runOnModule(Module &M) override;
    };

}

#endif //LLVM_NoVTAbiBuilder_H