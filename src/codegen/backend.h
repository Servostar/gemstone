
#ifndef CODEGN_BACKEND_H_
#define CODEGN_BACKEND_H_

#include <ast/ast.h>

typedef enum BackendError_t {
    Success,
    NoBackend,
    BackendAlreadySet,
    Other
} BackendError;

/**
 * @brief Function called by the compiler backend to generate an intermediate format
 *        from AST. Returns a custom container for its intermediate language.
 */
typedef size_t (*codegen)(const AST_NODE_PTR, void**);

/**
 * @brief Initialize the code generation backend.
 */
typedef size_t (*codegen_init)(void);

/**
 * @brief Undo initialization of code generation backend.
 */
typedef size_t (*codegen_deinit)(void);

/**
 * @brief Set the backend functions to use
 * 
 * @param init_func the function to call for initializing the backend 
 * @param deinit_func the function to call for undoing the initialization of the backend 
 * @param codegen_func the function to call when generating code
 * @param name name of the backend
 */
[[nodiscard]]
[[gnu::nonnull(1), gnu::nonnull(2), gnu::nonnull(3), gnu::nonnull(3)]]
BackendError set_backend(const codegen_init init_func, const codegen_deinit deinit_func, const codegen codegen_func, const char* name);

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
BackendError generate_code(const AST_NODE_PTR root, void** code);

#endif // CODEGN_BACKEND_H_
