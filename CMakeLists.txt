cmake_minimum_required(VERSION 2.8)

# Set these to point to the prefix under which the *installed* LLVM library can
# be found and the path to the LLVM source tree.
set(LLVM_PREFIX /usr)
set(LLVM_SRC_DIR ~/llvm-3.2)

set(LLVM_DIR ${LLVM_PREFIX}/share/llvm/cmake)
find_package(LLVM)
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

set(LLVM_LINK_COMPONENTS mcjit jit interpreter nativecodegen bitreader asmparser selectiondag native)
set(LLVM_BUILD_TOOLS true)
add_llvm_executable(trace
  trace.cpp
  ExtensibleInterpreter.cpp
)
set_target_properties(trace PROPERTIES 
    COMPILE_FLAGS "-fno-rtti -fvisibility-inlines-hidden"
)

# This allows our interpreter to include:
# ExecutionEngine/Interpreter/Interpreter.h
# which is not part of the installed headers.
include_directories(${LLVM_SRC_DIR}/lib)
