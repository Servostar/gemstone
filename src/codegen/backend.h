
#ifndef CODEGN_BACKEND_H_
#define CODEGN_BACKEND_H_

#include <ast/ast.h>
#include <cfg/opt.h>
#include <set/types.h>

typedef struct BackendImplError_t {
    // faulty AST node
    AST_NODE_PTR ast_node;
    // error message
    const char* message;
} BackendImplError;

typedef enum BackendErrorKind_t {
    Success,
    NoBackend,
    BackendAlreadySet,
    Unimplemented,
    // some error occurred in the backend implementation
    Implementation
} BackendErrorKind;

typedef struct BackendError_t {
    BackendErrorKind kind;
    BackendImplError impl;
} BackendError;

/**
 * @brief Function called by the compiler backend to generate an intermediate
 * format from AST. Returns a custom container for its intermediate language.
 */
typedef BackendError (*codegen)(const Module*, const TargetConfig* target);

/**
 * @brief Initialize the code generation backend.
 */
typedef BackendError (*codegen_init)(void);

/**
 * @brief Undo initialization of code generation backend.
 */
typedef BackendError (*codegen_deinit)(void);

/**
 * @brief Set the backend functions to use
 *
 * @param init_func the function to call for initializing the backend
 * @param deinit_func the function to call for undoing the initialization of the
 * backend
 * @param codegen_func the function to call when generating code
 * @param name name of the backend
 */
[[nodiscard]] [[gnu::nonnull(1), gnu::nonnull(2), gnu::nonnull(3),
                gnu::nonnull(3)]]
BackendError set_backend(const codegen_init init_func,
                         const codegen_deinit deinit_func,
                         const codegen codegen_func, const char* name);

/**
 * @brief Call the initialization function of the backend
 *
 * @return BackendError
 */
[[nodiscard]]
BackendError init_backend(void);

/**
 * @brief Call the undo initialization function of the backend
 *
 * @return BackendError
 */
[[nodiscard]]
BackendError deinit_backend(void);

/**
 * @brief Generate intermediate code with the registered backend
 *
 * @param root the root node of the AST
 * @param code output pointer to the intermediate code
 * @return BackendError
 */
[[nodiscard]]
BackendError generate_code(const Module* root, const TargetConfig* target);

/**
 * @brief Create a new backend error
 *
 * @param kind must ne != Implementation
 * @return BackendError
 */
BackendError new_backend_error(BackendErrorKind kind);

BackendError new_backend_impl_error(BackendErrorKind kind, AST_NODE_PTR node,
                                    const char* message);

#define SUCCESS new_backend_error(Success)

#endif // CODEGN_BACKEND_H_
