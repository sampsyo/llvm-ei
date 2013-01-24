#ifndef EXTENSIBLE_INTERPRETER_H
#define EXTENSIBLE_INTERPRETER_H

#include <vector>
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/IRReader.h"
#include "ExecutionEngine/Interpreter/Interpreter.h"

// This is a GIANT HACK that should NOT BE TRUSTED. I need access to the
// private fields of Interpreter, so here I'm redefining a similar-looking
// class and hoping that the compiler lays it out the same as Interpreter. Then
// I cast an Interpreter* to a PublicInterpreter* to access its
// otherwise-inaccessible fields.
//
// This will almost certainly break at some point.
class PublicInterpreter : public llvm::ExecutionEngine,
                          public llvm::InstVisitor<llvm::Interpreter> {
public:
  llvm::GenericValue ExitValue;
  llvm::DataLayout TD;
  llvm::IntrinsicLowering *IL;
  std::vector<llvm::ExecutionContext> ECStack;
  std::vector<llvm::Function*> AtExitHandlers;
};


// ExtensibleInterpreter is a facade around Interpreter that allows subclasses
// to override one important method, execute(). This method is responsible for
// interpreting a single instruction. Subclasses can override it to do whatever
// they like, including calling the superclass' version of execute() to make
// interpretation continue as usual.
class ExtensibleInterpreter : public llvm::ExecutionEngine {
public:
  llvm::Interpreter *interp;
  PublicInterpreter *pubInterp;  // GIANT HACK
  llvm::Module *module;

  explicit ExtensibleInterpreter(llvm::Module *M);
  virtual ~ExtensibleInterpreter();

  // Interpreter execution loop.
  virtual void run();
  virtual void execute(llvm::Instruction &I);

  // Convenience entry point.
  virtual int runMain(std::vector<std::string> args,
                      char * const *envp = 0);

  // Satisfy the ExecutionEngine interface.
  llvm::GenericValue runFunction(
      llvm::Function *F,
      const std::vector<llvm::GenericValue> &ArgValues
  );
  void *getPointerToNamedFunction(const std::string &Name,
                                  bool AbortOnFailure = true);
  void *recompileAndRelinkFunction(llvm::Function *F);
  void freeMachineCodeForFunction(llvm::Function *F);
  void *getPointerToFunction(llvm::Function *F);
  void *getPointerToBasicBlock(llvm::BasicBlock *BB);
};

// Utility for parsing and running a bitcode file with your interpreter.
template <typename InterpreterType>
int interpret(std::string bitcodeFile, std::vector<std::string> args,
              char * const *envp) {
  llvm::LLVMContext &Context = llvm::getGlobalContext();

  // Load the bitcode.
  llvm::SMDiagnostic Err;
  llvm::Module *Mod = llvm::ParseIRFile(bitcodeFile, Err, Context);
  if (!Mod) {
    llvm::errs() << "bitcode parsing failed\n";
    return -1;
  }

  // Load the whole bitcode file eagerly to check for errors.
  std::string ErrorMsg;
  if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
    llvm::errs() << "bitcode read error: " << ErrorMsg << "\n";
    return -1;
  }

  // Remove ".bc" suffix from input bitcode name and use it as argv[0].
  if (llvm::StringRef(bitcodeFile).endswith(".bc"))
    bitcodeFile.erase(bitcodeFile.length() - 3);
  args.insert(args.begin(), bitcodeFile);

  // Create and run the interpreter.
  ExtensibleInterpreter *interp = new InterpreterType(Mod);
  int retcode = interp->runMain(args, envp);
  delete interp;
  return retcode;
}

#endif
