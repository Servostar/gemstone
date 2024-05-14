
#ifndef _AST_H_
#define _AST_H_

#include <stdio.h>

/**
 * @brief The type of a AST node
 * @attention The last element is not to be used in the AST
 *            as it is used as a lazy way to get the total number of available
 *            variants of this enum.
 */
enum AST_SyntaxElement_t {
  AST_Stmt = 0,
  AST_Module,
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
  AST_Condition,
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
  // in this enums
  AST_List,
  AST_ExprList,
  AST_ArgList,
  AST_ParamList,
  AST_StmtList,
  AST_IdentList,
  AST_Storage,
  AST_Type,
  AST_Typekind,
  AST_Sign,
  AST_Scale,
  AST_Negate,
  AST_Parameter,
  AST_Qualifyier,
  AST_ParamDecl,
  AST_ELEMENT_COUNT
};

/**
 * @brief A single node which can be joined with other nodes like a graph.
 * Every node can have one ancestor (parent) but multiple (also none) children.
 * Nodes have two properties:
 *  - kind: The type of the node. Such as AST_Expr, AST_Add, ...
 *  - value: A string representing an optional value. Can be a integer literal for kind AST_int
 */
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

/**
 * Shorthand type for a single AST node
 */
typedef struct AST_Node_t* AST_NODE_PTR;

/**
 * @brief Initalize the global state of this module. Required for some functionality to work correctly.
 */
void AST_init(void);

/**
 * @brief Returns the string representation of the supplied node
 * @attention The retuned pointer is not to be freed as it may either be a statically stored string or
 *            used by the node after this function call.
 * @param node to return string representation of
 * @return string represenation of the node
 */
[[maybe_unused]]
[[gnu::nonnull(1)]]
const char* AST_node_to_string(const struct AST_Node_t* node);

/**
 * @brief Create a new node struct on the system heap. Initializes the struct with the given values.
 *        All other fields are set to either NULL or 0. No allocation for children array is preformed.
*  @attention parameter value can be NULL in case no value can be provided for the node
 * @param kind the type of this node
 * @param value an optional value for this node
 * @return
 */
[[maybe_unused]]
[[nodiscard("pointer must be freed")]]
[[gnu::returns_nonnull]]
struct AST_Node_t *AST_new_node(enum AST_SyntaxElement_t kind, const char* value);

/**
 * @brief Deallocate this node and all of its children.
 * @attention This function will detach this node from its parent if one is present
 *            Use of the supplied node after this call is undefined behavior
 * @param node The node to deallocate
 */
[[maybe_unused]]
[[gnu::nonnull(1)]]
void AST_delete_node(struct AST_Node_t * node);

/**
 * @brief Add a new child node to a parent node
 * @attention This can reallocate the children array
 * @param owner node to add a child to
 * @param child node to be added as a child
 */
[[maybe_unused]]
[[gnu::nonnull(1), gnu::nonnull(2)]]
void AST_push_node(struct AST_Node_t *owner, struct AST_Node_t *child);

/**
 * @brief Remove the specified child from the owner.
 * @attention The parent of the removed node is set to NULL.
 *            The returned pointer is still valid. It must be freed at some pointer later.
 * @param owner Node to remove the child from
 * @param idx the index of the child to remove
 * @return a pointer to the child which was removed
 */
[[maybe_unused]]
[[nodiscard("pointer must be freed")]]
[[gnu::nonnull(1)]]
struct AST_Node_t* AST_remove_child(struct AST_Node_t* owner, size_t idx);

/**
 * @brief Detach a child from its parent. This involves removing the child from its parent
 *        and marking the parent of the child as NULL.
 * @attention The returned pointer is still valid. It must be freed at some pointer later.
 * @param owner the owner to remove the child from
 * @param child the child to detach
 * @return a pointer to child detached
 */
[[nodiscard("pointer must be freed")]]
[[gnu::nonnull(1), gnu::nonnull(1)]]
struct AST_Node_t* AST_detach_child(struct AST_Node_t* owner, const struct AST_Node_t* child);

/**
 * @brief Return a pointer to the n-th child of a node
 * @attention Pointer to childen nodes will never change.
 *            However, the index a node is stored within a parent can change
 *            if a child of lower index is removed, thus reducing the childrens index by one.
 * @param owner the parent node which owns the children
 * @param idx the index of the child to get a pointer to
 * @return a pointer to the n-th child of the owner node
 */
[[maybe_unused]]
[[gnu::nonnull(1)]]
struct AST_Node_t *AST_get_node(struct AST_Node_t *owner, size_t idx);

/**
 * @brief Execute a function for every child, grandchild, ... and the supplied node as topmost ancestor
 * @param root the root to recursively execute a function for
 * @param for_each the function to execute
 */
[[maybe_unused]]
[[gnu::nonnull(1), gnu::nonnull(2)]]
void AST_visit_nodes_recurse(struct AST_Node_t *root,
                             void (*for_each)(struct AST_Node_t *node,
                                              size_t depth));

/**
 * @brief Prints a graphviz graph of the node and all its ancestors.
 * @param stream The stream to print to. Can be a file, stdout, ...
 * @param node the topmost ancestor
 */
[[maybe_unused]]
[[gnu::nonnull(1), gnu::nonnull(2)]]
void AST_fprint_graphviz(FILE* stream, const struct AST_Node_t* node);

#endif
