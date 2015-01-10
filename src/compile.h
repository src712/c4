#ifndef C4_COMPILE_H_
#define C4_COMPILE_H_

namespace llvm {
  class Module;
  class LLVMContext;
}
namespace c4 {

struct ast_node;
bool compile(ast_node*, llvm::Module&, llvm::LLVMContext&);

}

#endif /* C4_COMPILE_H_ */
