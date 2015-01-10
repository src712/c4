#ifndef C4_OPTIMIZE_H
#define C4_OPTIMIZE_H

namespace llvm {
class Module;
}

namespace c4 {

void optimize(llvm::Module&);

}

#endif /* C4_OPTIMIZE_H */
