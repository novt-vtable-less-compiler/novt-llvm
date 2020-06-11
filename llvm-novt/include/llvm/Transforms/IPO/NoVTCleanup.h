#ifndef LLVM_NOVTCLEANUP_H
#define LLVM_NOVTCLEANUP_H

#include <llvm/IR/PassManager.h>

#include <utility>

namespace llvm {

    struct NoVTCleanupPass : public PassInfoMixin<NoVTCleanupPass> {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
    };

    struct NoVTCleanupLegacyPass : public ModulePass {
        static char ID;

        NoVTCleanupLegacyPass();

        bool runOnModule(Module &M) override;

        void getAnalysisUsage(AnalysisUsage &usage) const override;

    };

}

#endif //LLVM_NOVTCLEANUP_H