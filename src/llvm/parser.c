
#include <llvm/parser.h>

BackendError parse_module(const Module* module, void**) {
    LLVMBackendCompileUnit* unit = malloc(sizeof(LLVMBackendCompileUnit));

    // we start with a LLVM module
    unit->context = LLVMContextCreate();
    unit->module = LLVMModuleCreateWithNameInContext("gemstone application", unit->context);

    

    LLVMDisposeModule(unit->module);
    LLVMContextDispose(unit->context);

    return new_backend_error(Success);
}
