
#ifndef _AST_H_
#define _AST_H_

#include <stdio.h>

struct AST_Node_t {
  // parent node that owns this node
  struct AST_Node_t *parent;

  // number of child nodes ownd by this node
  // length of children array
  size_t child_count;
  // variable amount of child nodes
  struct AST_Node_t **children;
};

// create a new initialized (empty) node
struct AST_Node_t *AST_new_node(void);

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

#endif
