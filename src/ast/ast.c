
#include <ast/ast.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/log.h>

struct AST_Node_t *AST_new_node(enum AST_SyntaxElement_t kind, const char* value) {
  DEBUG("creating new AST node: %d \"%s\"", kind, value);

  struct AST_Node_t *node = malloc(sizeof(struct AST_Node_t));

  if (node == NULL) {
    PANIC("failed to allocate AST node");
  }

  // init to discrete state
  node->parent = NULL;
  node->children = NULL;
  node->child_count = 0;
  node->kind = kind;
  node->value = value;

  return node;
}

static const char* lookup_table[AST_ELEMENT_COUNT] = { "__UNINIT__" };

void AST_init() {
  DEBUG("initializing global syntax tree...");

  INFO("filling lookup table...");

  lookup_table[AST_Stmt] = "stmt";
  lookup_table[AST_Expr] = "expr";

  lookup_table[AST_Add] = "+";
  lookup_table[AST_Sub] = "-";
  lookup_table[AST_Mul] = "*";
  lookup_table[AST_Div] = "/";

  lookup_table[AST_BitAnd] = "&";
  lookup_table[AST_BitOr] = "|";
  lookup_table[AST_BitXor] = "^";
  lookup_table[AST_BitNot] = "!";

  lookup_table[AST_Eq] = "==";
  lookup_table[AST_Less] = "<";
  lookup_table[AST_Greater] = ">";

  lookup_table[AST_BoolAnd] = "&&";
  lookup_table[AST_BoolOr] = "||";
  lookup_table[AST_BoolXor] = "^^";
  lookup_table[AST_BoolNot] = "!!";

  lookup_table[AST_While] = "while";
  lookup_table[AST_If] = "if";
  lookup_table[AST_IfElse] = "else if";
  lookup_table[AST_Else] = "else";

  lookup_table[AST_Decl] = "decl";
  lookup_table[AST_Assign] = "assign";
  lookup_table[AST_Def] = "def";

  lookup_table[AST_Typedef] = "typedef";
  lookup_table[AST_Box] = "box";
  lookup_table[AST_Fun] = "fun";

  lookup_table[AST_Typecast] = "cast";
  lookup_table[AST_Transmute] = "as";
  lookup_table[AST_Condition] = "condition";
}

const char* AST_node_to_string(struct AST_Node_t* node) {
  DEBUG("converting AST node to string: %p", node);

  const char* string = "unknown";

  switch(node->kind) {
    case AST_Int:
    case AST_Float:
    case AST_String:
    case AST_Ident:
    case AST_Macro:
    case AST_Import:
    case AST_Call:
      string = node->value;
      break;
    default:
      string = lookup_table[node->kind];
  }

  return string;
}

void AST_push_node(struct AST_Node_t *owner, struct AST_Node_t *child) {
  DEBUG("Adding new node %p to %p", child, owner);

  // if there are no children for now
  if (owner->child_count == 0) {
    DEBUG("Allocating new children array");
    owner->children = malloc(sizeof(struct AST_Node_t *));

  } else {
    DEBUG("Rellocating old children array");
    const size_t size = sizeof(struct AST_Node_t *) * (owner->child_count + 1);
    owner->children = realloc(owner->children, size);
  }

  if (owner->children == NULL) {
    PANIC("failed to allocate children array of AST node");
  }

  owner->children[owner->child_count++] = child;
}

struct AST_Node_t *AST_get_node(struct AST_Node_t *owner, size_t idx) {
  DEBUG("retrvieng node %d from %p", idx, owner);

  if (owner == NULL) {
    PANIC("AST owner node is NULL");
  }

  if (owner->children == NULL) {
    PANIC("AST owner node has no children");
  }

  struct AST_Node_t *child = owner->children[idx];

  if (child == NULL) {
    PANIC("child node is NULL");
  }

  return child;
}

struct AST_Node_t* AST_remove_child(struct AST_Node_t* owner, const size_t idx) {

  struct AST_Node_t* child = owner->children[idx];

  child->parent = NULL;

  owner->child_count--;

  // shift back every following element by one
  for (size_t i = idx; i < owner->child_count; i++) {
    owner->children[i] = owner->children[i + 1];
  }

  return child;
}

struct AST_Node_t* AST_detach_child(struct AST_Node_t* owner, const struct AST_Node_t* child) {
  for (size_t i = 0; i < owner->child_count; i++) {
    if (owner->children[i] == child) {
      return AST_remove_child(owner, i);
    }
  }

  PANIC("Child to detach not a child of parent");
  return NULL;
}

void AST_delete_node(struct AST_Node_t *node) {
  DEBUG("Deleting AST node: %p", node);

  if (node == NULL) {
    PANIC("Node to free is NULL");
  }

  if (node->children == NULL) {
    return;
  }

  if (node->parent != NULL) {
    AST_detach_child(node->parent, node);
  }

  for (size_t i = 0; i < node->child_count; i++) {
    // prevent detach of children node
    node->children[i]->parent = NULL;
    AST_delete_node(node->children[i]);
  }

  free(node->children);
  free(node);
}

static void __AST_visit_nodes_recurse2(struct AST_Node_t *root,
                                       void (*for_each)(struct AST_Node_t *node,
                                                        size_t depth),
                                       size_t depth) {
  DEBUG("Recursive visit of %p at %d with %p", root, depth, for_each);

  (for_each)(root, depth);

  for (size_t i = 0; i < root->child_count; i++) {
    __AST_visit_nodes_recurse2(root->children[i], for_each, depth + 1);
  }
}

void AST_visit_nodes_recurse(struct AST_Node_t *root,
                             void (*for_each)(struct AST_Node_t *node,
                                              size_t depth)) {
  DEBUG("Starting recursive visit of %p with %p", root, for_each);
  __AST_visit_nodes_recurse2(root, for_each, 0);
}

static void __AST_fprint_graphviz_node_definition(FILE* stream, struct AST_Node_t* node) {
  DEBUG("Printing graphviz definition of %p", node);

  fprintf(stream, "\tnode%p [label=\"%s\"]\n", (void*) node, AST_node_to_string(node));

  if (node->children == NULL) {
    return;
  }

  for (size_t i = 0; i < node->child_count; i++) {
    __AST_fprint_graphviz_node_definition(stream, node->children[i]);
  }
}

static void __AST_fprint_graphviz_node_connection(FILE* stream, struct AST_Node_t* node) {
  DEBUG("Printing graphviz connection of %p", node);

  if (node->children == NULL) {
    return;
  }

  for (size_t i = 0; i < node->child_count; i++) {
    fprintf(stream, "\tnode%p -- node%p\n", (void*) node, (void*) node->children[i]);
    __AST_fprint_graphviz_node_connection(stream, node->children[i]);
  }
}

void AST_fprint_graphviz(FILE* stream, struct AST_Node_t* root) {
  DEBUG("Starting print of graphviz graph of %p", root);

  fprintf(stream, "graph {\n");

  __AST_fprint_graphviz_node_definition(stream, root);
  __AST_fprint_graphviz_node_connection(stream, root);

  fprintf(stream, "}\n");
}
