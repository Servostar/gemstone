
#include <ast/ast.h>
#include <stdio.h>
#include <sys/log.h>
#include <assert.h>
#include <mem/cache.h>

struct AST_Node_t *AST_new_node(TokenLocation location, enum AST_SyntaxElement_t kind, const char* value) {
    DEBUG("creating new AST node: %d \"%s\"", kind, value);
    assert(kind < AST_ELEMENT_COUNT);

    struct AST_Node_t *node = mem_alloc(MemoryNamespaceAst, sizeof(struct AST_Node_t));

    if (node == NULL) {
        PANIC("failed to allocate AST node");
    }

    assert(node != NULL);

    // init to discrete state
    node->parent = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->kind = kind;
    node->value = value;
    node->location = location;

    return node;
}

static const char* lookup_table[AST_ELEMENT_COUNT] = { "__UNINIT__" };

void AST_init() {
    DEBUG("initializing global syntax tree...");

    INFO("filling lookup table...");
    lookup_table[AST_Stmt] = "stmt";
    lookup_table[AST_Module] = "module";
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

    lookup_table[AST_Call] = "funcall";
    lookup_table[AST_Typecast] = "typecast";
    lookup_table[AST_Transmute] = "transmute";
    lookup_table[AST_Condition] = "condition";
    lookup_table[AST_List] = "list";
    lookup_table[AST_ExprList] = "expr list";
    lookup_table[AST_ArgList] = "arg list";
    lookup_table[AST_ParamList] = "param list";
    lookup_table[AST_StmtList] = "stmt list";
    lookup_table[AST_IdentList] = "ident list";
    lookup_table[AST_Type] = "type";
    lookup_table[AST_Negate] = "-";
    lookup_table[AST_Parameter] = "parameter";
    lookup_table[AST_ParamDecl] = "parameter-declaration";
    lookup_table[AST_AddressOf] = "address of";
    lookup_table[AST_Dereference] = "deref";
    lookup_table[AST_Reference] = "ref";
}

const char* AST_node_to_string(const struct AST_Node_t* node) {
    DEBUG("converting AST node to string: %p", node);
    assert(node != NULL);

    const char* string;

    switch(node->kind) {
        case AST_Int:
        case AST_Float:
        case AST_String:
        case AST_Ident:
        case AST_Macro:
        case AST_Import:
        case AST_Storage:
        case AST_Typekind:
        case AST_Sign:
        case AST_Scale:
        case AST_Qualifyier:
            string = node->value;
            break;
        default:
            string = lookup_table[node->kind];
    }

    assert(string != NULL);

    return string;
}

static inline unsigned long int min(unsigned long int a, unsigned long int b) {
    return a > b ? b : a;
}

static inline unsigned long int max(unsigned long int a, unsigned long int b) {
    return a > b ? a : b;
}

void AST_push_node(struct AST_Node_t *owner, struct AST_Node_t *child) {
    DEBUG("Adding new node %p to %p", child, owner);
    assert(owner != NULL);
    assert(child != NULL);

    // if there are no children for now
    if (owner->child_count == 0) {
        DEBUG("Allocating new children array");
        owner->children = mem_alloc(MemoryNamespaceAst, sizeof(struct AST_Node_t *));

    } else {
        DEBUG("Rellocating old children array");
        const size_t size = sizeof(struct AST_Node_t *) * (owner->child_count + 1);
        owner->children = mem_realloc(MemoryNamespaceAst, owner->children, size);
    }

    if (owner->children == NULL) {
        PANIC("failed to allocate children array of AST node");
    }

    owner->location.col_end = max(owner->location.col_end, child->location.col_end);
    owner->location.line_end = max(owner->location.line_end, child->location.line_end);

    owner->location.col_start = min(owner->location.col_start, child->location.col_start);
    owner->location.line_start = min(owner->location.line_start, child->location.line_start);

    assert(owner->children != NULL);

    owner->children[owner->child_count++] = child;
}

struct AST_Node_t *AST_get_node(struct AST_Node_t *owner, const size_t idx) {
    DEBUG("retrvieng node %d from %p", idx, owner);
    assert(owner != NULL);
    assert(owner->children != NULL);
    assert(idx < owner->child_count);

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
    assert(owner != NULL);
    assert(owner->children != NULL);
    assert(idx < owner->child_count);

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
    assert(owner != NULL);
    assert(child != NULL);
    assert(owner->children != NULL);

    for (size_t i = 0; i < owner->child_count; i++) {
        if (owner->children[i] == child) {
            return AST_remove_child(owner, i);
        }
    }

    PANIC("Child to detach not a child of parent");
}

void AST_delete_node(struct AST_Node_t *node) {
    assert(node != NULL);

    DEBUG("Deleting AST node: %p", node);

    if (node->parent != NULL) {
        [[maybe_unused]]
        const struct AST_Node_t* child = AST_detach_child(node->parent, node);
        assert(child == node);
    }

    if (node->children != NULL) {
        for (size_t i = 0; i < node->child_count; i++) {
            // prevent detach of children node
            node->children[i]->parent = NULL;
            AST_delete_node(node->children[i]);
        }
        mem_free(node->children);
    }

    mem_free(node);
}

static void AST_visit_nodes_recurse2(struct AST_Node_t *root,
                                     void (*for_each)(struct AST_Node_t *node,
                                                      size_t depth),
                                     const size_t depth) {
    DEBUG("Recursive visit of %p at %d with %p", root, depth, for_each);

    assert(root != NULL);

    (for_each)(root, depth);

    for (size_t i = 0; i < root->child_count; i++) {
        AST_visit_nodes_recurse2(root->children[i], for_each, depth + 1);
    }
}

void AST_visit_nodes_recurse(struct AST_Node_t *root,
                             void (*for_each)(struct AST_Node_t *node,
                                              size_t depth)) {
    DEBUG("Starting recursive visit of %p with %p", root, for_each);

    assert(root != NULL);
    assert(for_each != NULL);

    AST_visit_nodes_recurse2(root, for_each, 0);
}

static void AST_fprint_graphviz_node_definition(FILE* stream, const struct AST_Node_t* node) {
    DEBUG("Printing graphviz definition of %p", node);

    assert(stream != NULL);
    assert(node != NULL);

    fprintf(stream, "\tnode%p [label=\"%s\"]\n", (void*) node, AST_node_to_string(node));

    if (node->children == NULL) {
        return;
    }

    for (size_t i = 0; i < node->child_count; i++) {
        AST_fprint_graphviz_node_definition(stream, node->children[i]);
    }
}

static void AST_fprint_graphviz_node_connection(FILE* stream, const struct AST_Node_t* node) {
    DEBUG("Printing graphviz connection of %p", node);

    assert(stream != NULL);
    assert(node != NULL);

    if (node->children == NULL) {
        return;
    }

    for (size_t i = 0; i < node->child_count; i++) {
        fprintf(stream, "\tnode%p -- node%p\n", (void*) node, (void*) node->children[i]);
        AST_fprint_graphviz_node_connection(stream, node->children[i]);
    }
}

void AST_fprint_graphviz(FILE* stream, const struct AST_Node_t* root) {
    DEBUG("Starting print of graphviz graph of %p", root);

    assert(stream != NULL);
    assert(root != NULL);

    fprintf(stream, "graph {\n");

    AST_fprint_graphviz_node_definition(stream, root);
    AST_fprint_graphviz_node_connection(stream, root);

    fprintf(stream, "}\n");
}
