#include <llvm/function/function.h>
#include <llvm/types/composite-types.h>
#include <llvm/types/structs.h>
#include <llvm/types/scope.h>
#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>
#include <sys/col.h>
#include <lex/util.h>
#include <llvm/backend.h>
#include <codegen/backend.h>
#include <llvm/types/type.h>
#include <assert.h>

extern FILE *yyin;
AST_NODE_PTR root;

/**
 * @brief Log a debug message to inform about beginning exit procedures
 *
 */
void notify_exit(void) { DEBUG("Exiting gemstone..."); }

/**
 * @brief Closes File after compiling.
 *
 */

void close_file(void) {
  if (NULL != yyin) {
    fclose(yyin);
  }
}

/**
 * @brief Run compiler setup here
 *
 */
void setup(void) {
  // setup preample

  log_init();
  DEBUG("starting gemstone...");

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
  atexit(&notify_exit);
#endif

  // actual setup
  AST_init();

  col_init();

  lex_init();

  DEBUG("finished starting up gemstone...");
}

void run_backend_codegen() {
  llvm_backend_init();

  BackendError err;
  err = init_backend();
  if (err.kind != Success) {
    return;
  }

  void* code = NULL;
  err = generate_code(root, &code);
  if (err.kind != Success) {
    return;
  }

  err = deinit_backend();
}

int main(int argc, char *argv[]) {

  setup();
  atexit(close_file);

  // Check for file input as argument
  if (2 != argc) {
    INFO("Usage: %s <filename>\n", argv[0]);
    PANIC("No File could be found");
  }

  // filename as first argument
  char *filename = argv[1];

  FILE *file = fopen(filename, "r");

  if (NULL == file) {
    PANIC("File couldn't be opened!");
  }
  yyin = file;

  root = AST_new_node(AST_Module, NULL);
  yyparse();

  TypeScopeRef scope = type_scope_new();

  AST_NODE_PTR fun = AST_get_node(root, 0);
  
  GemstoneFunRef function = fun_from_ast(scope, fun);
  type_scope_add_fun(scope, function);
  assert(function->params->len == 3);
  
  type_scope_delete(scope);

  AST_delete_node(root);
  return 0;
}