Extensible Interpreter for LLVM
===============================

In some recent projects, I've needed to do some simple dynamic analyses over compiled LLVM bitcode. The typical way to do this would be to instrument the code by inserting new instructions using an [LLVM pass][]. That can be tricky and error-prone, though, so here's another way.

LLVM ships with an [interpreter][llvm interpreter]. It's slow but it works. So why not just extend the interpreter to impose my own code during execution? That way I won't have to worry about generating bitcode instructions to get my runtime work done. I can stick to C++ and avoid linking in a runtime library to do so.

But LLVM's interpreter isn't meant to be extended in this way. (The crucial methods are non-virtual.) So I wrote this hack to provide a simple facade over the interpreter that *does* allow subclasses to interpose on the interpreter loop.

[llvm interpreter]: http://llvm.org/docs/CommandGuide/lli.html
[LLVM pass]: http://llvm.org/docs/WritingAnLLVMPass.html


A Demonstration
---------------

Included in this repository is a demonstration, in `trace.cpp`. To build it, first edit the top of `CMakeLists.txt` to point to LLVM's installation prefix and its source directory (you'll need to [download the source][llvm download]). These are the lines you need to change:

    set(LLVM_PREFIX /usr)
    set(LLVM_SRC_DIR ~/llvm-3.2)

Then build the example with the usual [CMake][] dance:

    $ cmake .
    $ make

You'll then have an executable called `trace` that works as an LLVM bitcode interpreter. To use it, compile any C file to bitcode. For example, you could make a file called test.c like this:
    
    int main(int argc, char *argv[]) {
        return argc + 1;
    }

Compile it to LLVM IR like so:

    $ clang -c -emit-llvm test.c -o test.bc

Then get an execution trace by running the interpreter:

    $ ./trace test.bc
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    %3 = alloca i8**, align 8
    store i32 0, i32* %1
    store i32 %argc, i32* %2, align 4
    store i8** %argv, i8*** %3, align 8
    %4 = load i32* %2, align 4
    %5 = add nsw i32 %4, 1
    ret i32 %5

Awesome!


Extending the Interpreter
-------------------------

If you open up `trace.cpp`, you'll see that you don't need much code to start extending the interpreter. There's some boilerplate to take the command-line arguments, but the bulk of the interpreter extension goes like this:

    class Tracer : public ExtensibleInterpreter {
    public:
        Tracer(Module *M) : ExtensibleInterpreter(M) {};
        virtual void execute(llvm::Instruction &I) {
            I.dump();
            ExtensibleInterpreter::execute(I);
        }
    };

Namely, you subclass `ExtensibleInterpreter` and override the `execute()` method. You probably want to call the superclass' version of that method to actually interpret the instruction, but you don't have to. Here, we just run `I.dump()` to output a description of each dynamic instruction we see. But you can do whatever you like during runtime, including changing the executed instructions on-the-fly.

To run the interpreter, use the provided `interpret()` function, which is paramaterized on the interpreter class:

    interpret<Tracer>(bitcodeFile, commandArgs, envp)

The three arguments are the bitcode file to execute, the command-line arguments (`argv`), and the environment (`envp`).


How it Works
------------

To make this work, I wrapped LLVM's `Interpreter` class in a new class that, via copypasta and other hacks, just passes control instruction-by-instruction to the real interpreter.

I needed access to private members of `Interpreter`, however, so I had to resort to a *terrible, incorrect* hack: I copied the definition of that class, made all its members public, and typecast the interpreter pointer to this new class. To access private `Interpreter` members in your code, just go through the `pubInterp` pointer. This hack will almost certainly break at some point, as it relies both on undefined compiler behavior and the internal structure of LLVM's classes. I'm sorry about that.


Author
------

This abomination is by [Adrian Sampson][home]. Use it under the terms of the same license as LLVM itself.


[home]: http://homes.cs.washington.edu/~asampson/
[CMake]: http://www.cmake.org/
[llvm download]: http://llvm.org/releases/download.html
