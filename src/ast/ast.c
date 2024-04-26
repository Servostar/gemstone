
#include <ast/ast.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/log.h>

struct AST_Node_t *AST_new_node(void) {
  struct AST_Node_t *node = malloc(sizeof(struct AST_Node_t));

  if (node == NULL) {
    PANIC("failed to allocate AST node");
  }

  // init to discrete state
  node->parent = NULL;
  node->children = NULL;
  node->child_count = 0;

  return node;
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

void AST_delete_node(struct AST_Node_t *_) {
#warning "FIXME: not implemented"
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
