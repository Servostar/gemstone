
#ifndef _AST_H_
#define _AST_H_

#include <stdio.h>

// Syntax elements which are stored in a syntax tree
enum AST_SyntaxElement_t {
  AST_Stmt = 0,
  AST_Expr,
  // Literals
  AST_Int,
  AST_Float,
  AST_String,
  // Control flow
  AST_While,
  AST_If,
  AST_IfElse,
  AST_Else,
  // Variable management
  AST_Decl,
  AST_Assign,
  AST_Def,
  AST_Ident,
  // Arithmetic operators
  AST_Add,
  AST_Sub,
  AST_Mul,
  AST_Div,
  // Bitwise operators
  AST_BitAnd,
  AST_BitOr,
  AST_BitXor,
  AST_BitNot,
  // Boolean operators
  AST_BoolAnd,
  AST_BoolOr,
  AST_BoolXor,
  AST_BoolNot,
  // Logical operators
  AST_Eq,
  AST_Greater,
  AST_Less,
  // Casts
  AST_Typecast,   // type cast
  AST_Transmute,  // reinterpret cast
  AST_Call,       // function call
  AST_Macro,      // builtin functions: lineno(), filename(), ...
  // Defintions
  AST_Typedef,
  AST_Box,
  AST_Fun,
  AST_Import,
  // amount of variants
  // in this enum
  AST_ELEMENT_COUNT
};

struct AST_Node_t {
  // parent node that owns this node
  struct AST_Node_t *parent;

  // type of AST node: if, declaration, ...
  enum AST_SyntaxElement_t kind;
  // optional value: integer literal, string literal, ...
  const char* value;

  // number of child nodes ownd by this node
  // length of children array
  size_t child_count;
  // variable amount of child nodes
  struct AST_Node_t **children;
};

typedef struct AST_Node_t* AST_NODE_PTR;

// initialize the global AST state
void AST_init(void);

// return a string representation of the nodes type and its value
// does not take into account its children or parent
const char* AST_node_to_string(struct AST_Node_t* node);

// create a new initialized (empty) node
struct AST_Node_t *AST_new_node(enum AST_SyntaxElement_t kind, const char* value);

void AST_delete_node(struct AST_Node_t *);

// add a new child node
void AST_push_node(struct AST_Node_t *owner, struct AST_Node_t *child);

// get a specific child node
struct AST_Node_t *AST_get_node(struct AST_Node_t *owner, size_t idx);

// visit this and all of its child nodes calling the given function
// for every node
void AST_visit_nodes_recurse(struct AST_Node_t *root,
                             void (*for_each)(struct AST_Node_t *node,
                                              size_t depth));

// print a graphviz diagram of the supplied node (as root node) into stream
void AST_fprint_graphviz(FILE* stream, struct AST_Node_t* node);

#endif
