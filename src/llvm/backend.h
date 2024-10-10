
#ifndef LLVM_CODEGEN_BACKEND_H_
#define LLVM_CODEGEN_BACKEND_H_

#include <llvm-c/TargetMachine.h>

enum StringAllocation_t { LLVM, LIBC, NONE };

typedef struct Target_t {
    char* name;
    char* triple;
    char* cpu;
    char* features;
    LLVMCodeGenOptLevel opt;
    LLVMRelocMode reloc;
    LLVMCodeModel model;
} Target;

Target create_native_target();

Target create_target_from_config(TargetConfig* config);

void delete_target(Target target);

void llvm_backend_init(void);

#endif // LLVM_CODEGEN_BACKEND_H_
