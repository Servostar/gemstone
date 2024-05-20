#include "llvm/types/scope.h"
#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>
#include <sys/col.h>
#include <lex/util.h>
#include <llvm/backend.h>
#include <codegen/backend.h>
#include <llvm/types/type.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

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

void check_typedef(const AST_NODE_PTR node) {
    TypeScopeRef scope = type_scope_new();

    GemstoneTypedefRef typdef = get_type_def_from_ast(scope, node);

    type_scope_delete(scope);
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

  for (size_t i = 0; i < root->child_count; i++) {
    check_typedef(root->children[i]);
  }

  AST_delete_node(root);
  return 0;
}
