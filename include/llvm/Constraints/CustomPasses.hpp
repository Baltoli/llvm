#ifndef _CUSTOM_PASSES_HPP_
#define _CUSTOM_PASSES_HPP_

namespace llvm {

class FunctionPass;
class ModulePass;

/* FlattenPass transforms all array accesses to be flat accesses to bare pointers.
   PreprocessorPass implements some additional peephole optimisations.
   ReplacerPass is the central part of this project, it uses the SMT based constraint solver to detect computational
   idioms in LLVM IR code. */
FunctionPass* createRemovePHIPass();
ModulePass*   createFlattenPass();
ModulePass*   createPreprocessorPass();
ModulePass*   createReplacerPass();
}

#endif
