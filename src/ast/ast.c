
#include <ast/ast.h>
#include <bits/posix2_lim.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/log.h>

struct AST_Node_t *AST_new_node(enum AST_SyntaxElement_t kind, const char* value) {
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

const char* AST_node_to_string(struct AST_Node_t* node) {
  const char* string = "unknown";

  switch (node->kind) {
    case AST_Expression:
      string = "expression";
      break;
    case AST_Statement:
      string = "statement";
      break;
    case AST_Branch:
      string = "if";
      break;
    case AST_IntegerLiteral:
      string = node->value;
      break;
    case AST_OperatorAdd:
      string = "+";
      break;
  }

  return string;
}

void AST_push_node(struct AST_Node_t *owner, struct AST_Node_t *child) {
  // if there are no children for now
  if (owner->child_count == 0) {
    owner->children = malloc(sizeof(struct AST_Node_t *));

  } else {
    size_t size = sizeof(struct AST_Node_t *) * (owner->child_count + 1);
    owner->children = realloc(owner->children, size);
  }

  if (owner->children == NULL) {
    PANIC("failed to allocate children array of AST node");
  }

  owner->children[owner->child_count++] = child;
}

struct AST_Node_t *AST_get_node(struct AST_Node_t *owner, size_t idx) {
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

void AST_delete_node(struct AST_Node_t *node) {
  if (node == NULL) {
    PANIC("Node to free is NULL");
  }

  if (node->children != NULL) {
    for (size_t i = 0; i < node->child_count; i++) {
      AST_delete_node(node->children[i]);
    }
  }
}

static void __AST_visit_nodes_recurse2(struct AST_Node_t *root,
                                       void (*for_each)(struct AST_Node_t *node,
                                                        size_t depth),
                                       size_t depth) {
  (for_each)(root, 0);

  for (size_t i = 0; i < root->child_count; i++) {
    __AST_visit_nodes_recurse2(root->children[i], for_each, depth + 1);
  }
}

void AST_visit_nodes_recurse(struct AST_Node_t *root,
                             void (*for_each)(struct AST_Node_t *node,
                                              size_t depth)) {
  __AST_visit_nodes_recurse2(root, for_each, 0);
}

void AST_fprint_graphviz_node_definition(FILE* stream, struct AST_Node_t* node) {

  fprintf(stream, "\tnode%p [label=\"%s\"]\n", (void*) node, AST_node_to_string(node));

  if (node->children != NULL) {
    for (size_t i = 0; i < node->child_count; i++) {
      AST_fprint_graphviz_node_definition(stream, node->children[i]);
    }
  }
}

void AST_fprint_graphviz_node_connection(FILE* stream, struct AST_Node_t* node) {

  if (node->children != NULL) {
    for (size_t i = 0; i < node->child_count; i++) {
      fprintf(stream, "\tnode%p -- node%p\n", (void*) node, (void*) node->children[i]);
      AST_fprint_graphviz_node_connection(stream, node->children[i]);
    }
  }
}

void AST_fprint_graphviz(FILE* stream, struct AST_Node_t* root) {
  fprintf(stream, "graph {\n");

  AST_fprint_graphviz_node_definition(stream, root);
  AST_fprint_graphviz_node_connection(stream, root);

  fprintf(stream, "}\n");
}
