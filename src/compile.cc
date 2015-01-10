#include "compile.h"
#include "ast.h"
#include "type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/Host.h"

namespace c4 {
bool compile(ast_node* ast, llvm::Module& m,
             llvm::LLVMContext& ctx) {
  llvm::IRBuilder<> builder(ctx), alloca_builder(ctx);
  auto tu = dynamic_cast<translation_unit*>(ast);

  for(auto& d : tu->decls()) {
    d->gen_code(m, builder, alloca_builder, nullptr);
  }
  
  m.setTargetTriple(llvm::sys::getDefaultTargetTriple());
  return !llvm::verifyModule(m);
}

}
