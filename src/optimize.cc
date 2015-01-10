#include <iostream>

#include "optimize.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/InstIterator.h"

#include "llvm/Target/TargetLibraryInfo.h"

#include <set>

namespace llvm {
void initializeconstant_propagationPass(PassRegistry&);
}

namespace {
using namespace llvm;
struct constant_propagation : public FunctionPass {
  static char ID;
  constant_propagation() : FunctionPass(ID) {
    initializeconstant_propagationPass(*PassRegistry::getPassRegistry());
  }
  
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetLibraryInfo>();
  }

  bool runOnFunction(Function& F) {
    std::set<Instruction*> work_list;
    for(auto i = inst_begin(F), e = inst_end(F); i != e; ++i) {
      /* if(this->value(&*i) != TOP) */
        work_list.insert(&*i);
    }

    bool changed = false;
    DataLayout *TD = getAnalysisIfAvailable<DataLayout>();
    TargetLibraryInfo *TLI = &getAnalysis<TargetLibraryInfo>();

    while(!work_list.empty()) {
      Instruction *I = *work_list.begin();
      work_list.erase(work_list.begin());
      if(!I->use_empty()) {
        if (Constant *C = ConstantFoldInstruction(I, TD, TLI)) {

          for (Value::use_iterator UI = I->use_begin(), UE = I->use_end();
               UI != UE; ++UI)
            work_list.insert(cast<Instruction>(*UI));

          I->replaceAllUsesWith(C);

          work_list.erase(I);
          I->eraseFromParent();

          changed = true;
        }
      } else { // dead instructions
        
      }
    }

    return changed;
  }

private:
  enum Lattice {
    TOP, /* value not known */
    C, /* value known */
    BOT /* value known to vary */
  };

  Lattice value(Instruction* inst) {
    (void)inst;
    return BOT;
  }
};
}

char constant_propagation::ID = 0;
INITIALIZE_PASS_BEGIN(constant_propagation, "constant_propagation",
                      "Simple constant propagation", false, false)
INITIALIZE_PASS_DEPENDENCY(DataLayout)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfo)
INITIALIZE_PASS_END(constant_propagation, "constant_propagation",
                    "Simple constant propagation", false, false)

namespace c4 {
void optimize(llvm::Module& m) {
  using namespace llvm;
  PassManager pm;
  pm.add(createPromoteMemoryToRegisterPass());
  pm.add(new constant_propagation());
  pm.run(m);
}
}
