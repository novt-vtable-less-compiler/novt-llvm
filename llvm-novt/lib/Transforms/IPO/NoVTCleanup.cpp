#include "llvm/Transforms/IPO/NoVTCleanup.h"

using namespace llvm;

namespace {

    bool cleanupModule(Module &M) {
      long linkage_changed = 0;

      for (auto &alias: M.aliases()) {
        // is an alias?
        if (alias.hasName() && alias.getAliasee()) {
          // work here
          if (isa<Function>(alias.getAliasee())) {
            auto f = cast<Function>(alias.getAliasee());
            if (llvm::Function::isAvailableExternallyLinkage(f->getLinkage())) {
              f->setLinkage(llvm::Function::InternalLinkage);
              linkage_changed++;
            }
          }
        }
      }

      llvm::errs() << "[Cleanup] Alias anti-bug changed " << linkage_changed << " aliases\n";
      return linkage_changed > 0;
    }

}

PreservedAnalyses NoVTCleanupPass::run(Module &M, ModuleAnalysisManager &MAM) {
  return cleanupModule(M) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool NoVTCleanupLegacyPass::runOnModule(Module &M) {
  return cleanupModule(M);
}

char NoVTCleanupLegacyPass::ID = 0;

NoVTCleanupLegacyPass::NoVTCleanupLegacyPass() : ModulePass(ID) {}

void NoVTCleanupLegacyPass::getAnalysisUsage(AnalysisUsage &AU) const {
}

static RegisterPass<NoVTCleanupLegacyPass> Registration("NoVTCleanup", "NoVTCleanupLegacyPass", false, false);
