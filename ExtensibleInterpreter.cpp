#include "ExtensibleInterpreter.h"
#include <cerrno>

using namespace llvm;

ExtensibleInterpreter::ExtensibleInterpreter(Module *M) :
      ExecutionEngine(M),
      module(M)
{
  interp = new Interpreter(M);
  pubInterp = (PublicInterpreter *)interp;  // GIANT HACK

  // Data layout is used by ExecutionEngine::runFunctionAsMain().
  setDataLayout(interp->getDataLayout());

  // This is hacky: remove M from the ExecutionEngine's module list to avoid a
  // double-delete when both it and the Interpreter are destructed.
  Modules.clear();
}

ExtensibleInterpreter::~ExtensibleInterpreter() {
  delete interp;
}


// The main interpreter loop.

void ExtensibleInterpreter::run() {
  while (!pubInterp->ECStack.empty()) {
    ExecutionContext &SF = pubInterp->ECStack.back();
    Instruction &I = *SF.CurInst++;
    execute(I);
  }
}

void ExtensibleInterpreter::execute(Instruction &I) {
  interp->visit(I);
}


// Convenient entry point.

int ExtensibleInterpreter::runMain(std::vector<std::string> args,
                                   char * const *envp) {
  // Get the main function from the module.
  Function *EntryFn = module->getFunction("main");
  if (!EntryFn) {
    errs() << '\'' << "main" << "\' function not found in module.\n";
    return -1;
  }

  // Reset errno to zero on entry to main.
  errno = 0;

  // Run main.
  return runFunctionAsMain(EntryFn, args, envp);
}


// The ExecutionEngine interface.

GenericValue ExtensibleInterpreter::runFunction(
    Function *F,
    const std::vector<GenericValue> &ArgValues
) {
  // Copied & pasted from Interpreter, essentially.
  std::vector<GenericValue> ActualArgs;
  const unsigned ArgCount = F->getFunctionType()->getNumParams();
  for (unsigned i = 0; i < ArgCount; ++i)
    ActualArgs.push_back(ArgValues[i]);
  interp->callFunction(F, ActualArgs);
  run();
  return pubInterp->ExitValue;
}
void *ExtensibleInterpreter::getPointerToNamedFunction(
    const std::string &Name,
    bool AbortOnFailure
) {
  return 0;
}
void *ExtensibleInterpreter::recompileAndRelinkFunction(Function *F) {
  return getPointerToFunction(F);
}
void ExtensibleInterpreter::freeMachineCodeForFunction(Function *F) {
}
void *ExtensibleInterpreter::getPointerToFunction(Function *F) {
  return (void*)F;
}
void *ExtensibleInterpreter::getPointerToBasicBlock(BasicBlock *BB) {
  return (void*)BB;
}
