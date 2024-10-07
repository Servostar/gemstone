
#include <assert.h>
#include <ast/ast.h>
#include <mem/cache.h>
#include <stdio.h>
#include <sys/log.h>

struct AST_Node_t* AST_new_node(TokenLocation location,
                                enum AST_SyntaxElement_t kind,
                                const char* value) {
    DEBUG("creating new AST node: %d \"%s\"", kind, value);
    assert(kind < AST_ELEMENT_COUNT);

    struct AST_Node_t* node =
      mem_alloc(MemoryNamespaceAst, sizeof(struct AST_Node_t));

    if (node == NULL) {
        PANIC("failed to allocate AST node");
    }

    assert(node != NULL);

    // init to discrete state
    node->parent   = NULL;
    node->children = mem_new_g_array(MemoryNamespaceAst, sizeof(AST_NODE_PTR));
    node->kind     = kind;
    node->value    = value;
    node->location = location;

    return node;
}

static const char* lookup_table[AST_ELEMENT_COUNT] = {"__UNINIT__"};

void AST_init() {
    DEBUG("initializing global syntax tree...");

    INFO("filling lookup table...");
    lookup_table[AST_Stmt]   = "stmt";
    lookup_table[AST_Module] = "module";
    lookup_table[AST_Expr]   = "expr";

    lookup_table[AST_Add] = "+";
    lookup_table[AST_Sub] = "-";
    lookup_table[AST_Mul] = "*";
    lookup_table[AST_Div] = "/";

    lookup_table[AST_BitAnd] = "&";
    lookup_table[AST_BitOr]  = "|";
    lookup_table[AST_BitXor] = "^";
    lookup_table[AST_BitNot] = "!";

    lookup_table[AST_Eq]      = "==";
    lookup_table[AST_Less]    = "<";
    lookup_table[AST_Greater] = ">";

    lookup_table[AST_BoolAnd] = "&&";
    lookup_table[AST_BoolOr]  = "||";
    lookup_table[AST_BoolXor] = "^^";
    lookup_table[AST_BoolNot] = "!!";

    lookup_table[AST_While]  = "while";
    lookup_table[AST_If]     = "if";
    lookup_table[AST_IfElse] = "else if";
    lookup_table[AST_Else]   = "else";

    lookup_table[AST_Decl]   = "decl";
    lookup_table[AST_Assign] = "assign";
    lookup_table[AST_Def]    = "def";

    lookup_table[AST_Typedef]  = "typedef";
    lookup_table[AST_Box]      = "box";
    lookup_table[AST_FunDecl]  = "fundef";
    lookup_table[AST_FunDef]   = "fundecl";
    lookup_table[AST_ProcDecl] = "procdef";
    lookup_table[AST_ProcDef]  = "procdef";

    lookup_table[AST_Call]        = "funcall";
    lookup_table[AST_Typecast]    = "typecast";
    lookup_table[AST_Transmute]   = "transmute";
    lookup_table[AST_Condition]   = "condition";
    lookup_table[AST_List]        = "list";
    lookup_table[AST_ExprList]    = "expr list";
    lookup_table[AST_ArgList]     = "arg list";
    lookup_table[AST_ParamList]   = "param list";
    lookup_table[AST_StmtList]    = "stmt list";
    lookup_table[AST_IdentList]   = "ident list";
    lookup_table[AST_Type]        = "type";
    lookup_table[AST_Negate]      = "-";
    lookup_table[AST_Parameter]   = "parameter";
    lookup_table[AST_ParamDecl]   = "parameter-declaration";
    lookup_table[AST_AddressOf]   = "address of";
    lookup_table[AST_Dereference] = "deref";
    lookup_table[AST_Reference]   = "ref";
    lookup_table[AST_Return]      = "ret";
}

const char* AST_node_to_string(const struct AST_Node_t* node) {
    DEBUG("converting AST node to string: %p", node);
    assert(node != NULL);

    const char* string;

    switch (node->kind) {
        case AST_Int:
        case AST_Char:
        case AST_Float:
        case AST_String:
        case AST_Ident:
        case AST_Macro:
        case AST_Import:
        case AST_Include:
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

void AST_push_node(struct AST_Node_t* owner, struct AST_Node_t* child) {
    DEBUG("Adding new node %p to %p", child, owner);
    assert(owner != NULL);
    assert(child != NULL);

    if (owner->children == NULL) {
        PANIC("failed to allocate children array of AST node");
    }

    owner->location.col_end =
      max(owner->location.col_end, child->location.col_end);
    owner->location.line_end =
      max(owner->location.line_end, child->location.line_end);

    owner->location.col_start =
      min(owner->location.col_start, child->location.col_start);
    owner->location.line_start =
      min(owner->location.line_start, child->location.line_start);

    if (owner->location.file == NULL) {
        owner->location.file = child->location.file;
    }

    assert(owner->children != NULL);

    g_array_append_val(owner->children, child);
}

struct AST_Node_t* AST_get_node(struct AST_Node_t* owner, const size_t idx) {
    DEBUG("retrvieng node %d from %p", idx, owner);
    assert(owner != NULL);
    assert(owner->children != NULL);
    assert(idx < owner->children->len);

    if (owner->children == NULL) {
        PANIC("AST owner node has no children");
    }

    AST_NODE_PTR child = g_array_index(owner->children, AST_NODE_PTR, idx);

    if (child == NULL) {
        PANIC("child node is NULL");
    }

    return child;
}

struct AST_Node_t* AST_remove_child(struct AST_Node_t* owner,
                                    const size_t idx) {
    assert(owner != NULL);
    assert(owner->children != NULL);
    assert(idx < owner->children->len);

    AST_NODE_PTR child = g_array_index(owner->children, AST_NODE_PTR, idx);
    g_array_remove_index(owner->children, idx);

    child->parent = NULL;

    return child;
}

struct AST_Node_t* AST_detach_child(struct AST_Node_t* owner,
                                    const struct AST_Node_t* child) {
    assert(owner != NULL);
    assert(child != NULL);
    assert(owner->children != NULL);

    for (size_t i = 0; i < owner->children->len; i++) {
        if (g_array_index(owner->children, AST_NODE_PTR, i) == child) {
            return AST_remove_child(owner, i);
        }
    }

    PANIC("Child to detach not a child of parent");
}

void AST_delete_node(struct AST_Node_t* node) {
    assert(node != NULL);

    DEBUG("Deleting AST node: %p", node);

    if (node->parent != NULL) {
        [[maybe_unused]] const struct AST_Node_t* child =
          AST_detach_child(node->parent, node);
        assert(child == node);
    }

    if (node->children != NULL) {
        for (size_t i = 0; i < AST_get_child_count(node); i++) {
            // prevent detach of children node
            AST_get_node(node, i)->parent = NULL;
            AST_delete_node(AST_get_node(node, i));
        }
        mem_free(node->children);
    }

    mem_free(node);
}

static void AST_visit_nodes_recurse2(struct AST_Node_t* root,
                                     void (*for_each)(struct AST_Node_t* node,
                                                      size_t depth),
                                     const size_t depth) {
    DEBUG("Recursive visit of %p at %d with %p", root, depth, for_each);

    assert(root != NULL);

    (for_each)(root, depth);

    for (size_t i = 0; i < root->children->len; i++) {
        AST_visit_nodes_recurse2(g_array_index(root->children, AST_NODE_PTR, i),
                                 for_each, depth + 1);
    }
}

void AST_visit_nodes_recurse(struct AST_Node_t* root,
                             void (*for_each)(struct AST_Node_t* node,
                                              size_t depth)) {
    DEBUG("Starting recursive visit of %p with %p", root, for_each);

    assert(root != NULL);
    assert(for_each != NULL);

    AST_visit_nodes_recurse2(root, for_each, 0);
}

static void AST_fprint_graphviz_node_definition(FILE* stream,
                                                const struct AST_Node_t* node) {
    DEBUG("Printing graphviz definition of %p", node);

    assert(stream != NULL);
    assert(node != NULL);

    char* module_path = module_ref_to_str(node->location.module_ref);
    fprintf(stream, "\tnode%p [label=\"@%s\\n%s\"]\n", (void*) node, module_path,
            AST_node_to_string(node));

    if (node->children == NULL) {
        return;
    }

    for (size_t i = 0; i < node->children->len; i++) {
        AST_fprint_graphviz_node_definition(
          stream, g_array_index(node->children, AST_NODE_PTR, i));
    }
}

static void AST_fprint_graphviz_node_connection(FILE* stream,
                                                const struct AST_Node_t* node) {
    DEBUG("Printing graphviz connection of %p", node);

    assert(stream != NULL);
    assert(node != NULL);

    if (node->children == NULL) {
        return;
    }

    for (size_t i = 0; i < node->children->len; i++) {
        AST_NODE_PTR child = g_array_index(node->children, AST_NODE_PTR, i);
        fprintf(stream, "\tnode%p -- node%p\n", (void*) node, (void*) child);
        AST_fprint_graphviz_node_connection(stream, child);
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

AST_NODE_PTR AST_get_node_by_kind(AST_NODE_PTR owner,
                                  enum AST_SyntaxElement_t kind) {
    for (size_t i = 0; i < owner->children->len; i++) {
        AST_NODE_PTR child = AST_get_node(owner, i);

        if (child->kind == kind) {
            return child;
        }
    }

    return NULL;
}

void AST_merge_modules(AST_NODE_PTR dst, size_t k, AST_NODE_PTR src) {
    assert(dst != NULL);
    assert(src != NULL);

    size_t elements = src->children->len;
    for (size_t i = 0; i < elements; i++) {
        AST_insert_node(dst, k + i, AST_remove_child(src, 0));
    }
    AST_delete_node(src);
}

void AST_import_module(AST_NODE_PTR dst, size_t k, AST_NODE_PTR src) {
    assert(dst != NULL);
    assert(src != NULL);

    size_t d = 0;

    size_t elements = src->children->len;
    for (size_t i = 0; i < elements; i++) {
        AST_NODE_PTR node = AST_remove_child(src, 0);

        // TODO: resolve by public symbols
        switch (node->kind) {
            case AST_FunDef:
            {
                AST_NODE_PTR decl = AST_new_node(node->location, AST_FunDecl, NULL);

                for (int u = 0; u < node->children->len - 1; u++) {
                    AST_push_node(decl, AST_get_node(node, u));
                }

                node = decl;
                break;
            }
            case AST_ProcDef:
            {
                AST_NODE_PTR decl = AST_new_node(node->location, AST_ProcDecl, NULL);

                for (int u = 0; u < node->children->len - 1; u++) {
                    AST_push_node(decl, AST_get_node(node, u));
                }

                node = decl;
                break;
            }
            case AST_Typedef:
            case AST_Def:
                break;
            default:
                node = NULL;
        }

        if (node != NULL)  {
            AST_insert_node(dst, k + d, node);
            d++;
        }
    }
    AST_delete_node(src);
}

void AST_insert_node(AST_NODE_PTR owner, size_t idx, AST_NODE_PTR child) {
    assert(owner != NULL);
    assert(child != NULL);

    DEBUG("Reallocating old children array");

    g_array_insert_val(owner->children, idx, child);
}

size_t AST_get_child_count(AST_NODE_PTR node) {
    return node->children->len;
}

AST_NODE_PTR AST_get_last_node(AST_NODE_PTR node) {
    assert(node != NULL);

    return g_array_index(node->children, AST_NODE_PTR, node->children->len - 1);
}
