#include "llvm/types/composite-types.h"
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

void check_type(const TypeScopeRef scope, const char* name, enum Sign_t sign, enum Scale_t scale, enum Primitive_t prim) {
  GemstoneTypedefRef type = type_scope_get_type_from_name(scope, name);
  INFO("Expected: %d %d %d Given: %d %d %d", sign, scale, prim, type->type->specs.composite.sign, type->type->specs.composite.scale, type->type->specs.composite.prim);
  assert(type->type->kind == TypeComposite);
  assert(type->type->specs.composite.prim == prim);
  assert(type->type->specs.composite.scale == scale);
  assert(type->type->specs.composite.sign == sign);
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

  for (size_t i = 0; i < root->child_count; i++) {
    GemstoneTypedefRef typdef = get_type_def_from_ast(scope, root->children[i]);

    type_scope_append_type(scope, typdef);
  }
  
  check_type(scope, "u8", Unsigned, ATOM, Int);
  check_type(scope, "u16", Unsigned, HALF, Int);
  check_type(scope, "u32", Unsigned, SINGLE, Int);
  check_type(scope, "u64", Unsigned, DOUBLE, Int);
  check_type(scope, "u128", Unsigned, QUAD, Int);
  check_type(scope, "u256", Unsigned, OCTO, Int);

  check_type(scope, "i8", Signed, ATOM, Int);
  check_type(scope, "i16", Signed, HALF, Int);
  check_type(scope, "i32", Signed, SINGLE, Int);
  check_type(scope, "i64", Signed, DOUBLE, Int);
  check_type(scope, "i128", Signed, QUAD, Int);
  check_type(scope, "i256", Signed, OCTO, Int);

  check_type(scope, "short_int", Signed, HALF, Int);

  check_type(scope, "f16", Signed, HALF, Float);
  check_type(scope, "f32", Signed, SINGLE, Float);
  check_type(scope, "f64", Signed, DOUBLE, Float);
  check_type(scope, "f128", Signed, QUAD, Float);

  type_scope_delete(scope);

  AST_delete_node(root);
  return 0;
}