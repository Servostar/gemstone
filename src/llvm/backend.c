
#include <llvm/decl/variable.h>
#include <llvm/function/function-types.h>
#include <llvm/function/function.h>
#include <llvm/types/scope.h>
#include <llvm/types/structs.h>
#include <llvm/types/type.h>
#include <codegen/backend.h>
#include <sys/log.h>
#include <ast/ast.h>
#include <llvm/backend.h>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>

typedef enum LLVMBackendError_t {
    UnresolvedImport
} LLVMBackendError;

static BackendError llvm_backend_codegen(const AST_NODE_PTR module_node, void**) {
    // we start with a LLVM module
    LLVMContextRef context = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext("gemstone application", context);

    BackendError err;

    TypeScopeRef global_scope = type_scope_new();

    for (size_t i = 0; i < module_node->child_count; i++) {
        // iterate over all nodes in module
        // can either be a function, box, definition, declaration or typedefine

        AST_NODE_PTR global_node = AST_get_node(module_node, i);

        GemstoneTypedefRef typedefref;
        GArray* decls;

        switch (global_node->kind) {
            case AST_Typedef:
                typedefref = get_type_def_from_ast(global_scope, global_node);
                type_scope_append_type(global_scope, typedefref);
                break;
            case AST_Fun:
                llvm_generate_function_implementation(global_scope, module, global_node);
                break;
            case AST_Decl:
                decls = declaration_from_ast(global_scope, global_node);
                for (size_t i = 0; i < decls->len; i++) {
                    GemstoneDeclRef decl = ((GemstoneDeclRef*) decls->data)[i];
                    
                    err = llvm_create_declaration(module, NULL, decl, &decl->llvm_value);

                    if (err.kind != Success)
                        break;
                    
                    type_scope_add_variable(global_scope, decl);
                }

                break;
            default:
                PANIC("NOT IMPLEMENTED");
        }
    }

    type_scope_delete(global_scope);

    LLVMDisposeModule(module);
    LLVMContextDispose(context);

    return new_backend_error(Success);
}

static BackendError llvm_backend_codegen_init(void) {
    return new_backend_error(Success);
}

static BackendError llvm_backend_codegen_deinit(void) {
    return new_backend_error(Success);
}

void llvm_backend_init() {
    BackendError err = set_backend(&llvm_backend_codegen_init, &llvm_backend_codegen_deinit, &llvm_backend_codegen, "LLVM");

    if (err.kind != Success) {
        PANIC("unable to init llvm backend: %ld", err);
    }
}
