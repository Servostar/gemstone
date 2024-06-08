#include <io/files.h>
#include <yacc/parser.tab.h>
#include <complex.h>
#include <stdio.h>
#include <ast/ast.h>
#include <set/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/log.h>
#include <glib.h>
#include <assert.h>
#include <math.h>
#include <set/set.h>
#include <mem/cache.h>

extern ModuleFile *current_file;
static GHashTable *declaredComposites = NULL;//pointer to composites with names 
static GHashTable *declaredBoxes = NULL;//pointer to type
static GHashTable *functionParameter = NULL;
static GHashTable *definedFunctions = NULL;
static GHashTable *declaredFunctions = NULL;
static GArray *Scope = NULL;//list of hashtables. last Hashtable is current depth of program. hashtable key: ident, value: Variable* to var



const Type ShortShortUnsingedIntType = {
        .kind = TypeKindComposite,
        .impl = {
                .composite = {
                        .sign = Unsigned,
                        .scale = 0.25,
                        .primitive = Int
                }
        },
        .nodePtr = NULL,
};

const Type StringLiteralType = {
        .kind = TypeKindReference,
        .impl = {
                .reference = (ReferenceType) &ShortShortUnsingedIntType,
        },
        .nodePtr = NULL,
};

/**
 * @brief Convert a string into a sign typ
 * @return 0 on success, 1 otherwise
 */
int sign_from_string(const char *string, Sign *sign) {
    assert(string != NULL);
    assert(sign != NULL);

    if (strcmp(string, "unsigned") == 0) {
        *sign = Unsigned;
        return SEMANTIC_OK;
    }

    if (strcmp(string, "signed") == 0) {
        *sign = Signed;
        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

/**
 * @brief Convert a string into a primitive type
 * @return 0 on success, 1 otherwise
 */
int primitive_from_string(const char *string, PrimitiveType *primitive) {
    assert(string != NULL);
    assert(primitive != NULL);
    DEBUG("find primitive in string");

    if (strcmp(string, "int") == 0) {
        *primitive = Int;
        return SEMANTIC_OK;
    }

    if (strcmp(string, "float") == 0) {
        *primitive = Float;
        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

int scale_factor_from(const char *string, double *factor) {
    assert(string != NULL);
    assert(factor != NULL);

    if (strcmp(string, "half") == 0 || strcmp(string, "short") == 0) {
        *factor = 0.5;
        return SEMANTIC_OK;
    }

    if (strcmp(string, "double") == 0 || strcmp(string, "long") == 0) {
        *factor = 2.0;
        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

int check_scale_factor(AST_NODE_PTR node, Scale scale) {
    assert(node != NULL);

    if (8 < scale) {
        print_diagnostic(current_file, &node->location, Error, "Composite scale overflow");
        return SEMANTIC_ERROR;
    }

    if (0.25 > scale) {
        print_diagnostic(current_file, &node->location, Error, "Composite scale underflow");
        return SEMANTIC_ERROR;
    }

    return SEMANTIC_OK;
}

int merge_scale_list(AST_NODE_PTR scale_list, Scale *scale) {
    assert(scale_list != NULL);
    assert(scale != NULL);

    for (size_t i = 0; i < scale_list->child_count; i++) {

        double scale_in_list = 1.0;
        int scale_invalid = scale_factor_from(AST_get_node(scale_list, i)->value, &scale_in_list);

        if (scale_invalid == SEMANTIC_ERROR) {
            return SEMANTIC_ERROR;
        }

        *scale *= scale_in_list;
    }

    return SEMANTIC_OK;
}

/**
 * @brief Get an already declared type from its name
 */
int get_type_decl(const char *name, Type **type) {
    assert(name != NULL);
    assert(type != NULL);

    if (g_hash_table_contains(declaredComposites, name) == TRUE) {

        *type = (Type *) g_hash_table_lookup(declaredComposites, name);

        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

int impl_composite_type(AST_NODE_PTR ast_type, CompositeType *composite) {
    assert(ast_type != NULL);
    assert(composite != NULL);

    DEBUG("Type is a Composite");

    int status = SEMANTIC_OK;
    int scaleNodeOffset = 0;

    composite->sign = Signed;

    // check if we have a sign
    if (AST_Sign == ast_type->children[0]->kind) {

        status = sign_from_string(ast_type->children[0]->value, &composite->sign);

        if (status == SEMANTIC_ERROR) {
            ERROR("invalid sign: %s", ast_type->children[0]->value);
            return SEMANTIC_ERROR;
        }

        scaleNodeOffset++;
    }

    composite->scale = 1.0;

    // check if we have a list of scale factors
    if (ast_type->children[scaleNodeOffset]->kind == AST_List) {

        status = merge_scale_list(ast_type->children[scaleNodeOffset], &composite->scale);

        if (status == SEMANTIC_ERROR) {
            return SEMANTIC_ERROR;
        }
    }

    AST_NODE_PTR typeKind = ast_type->children[ast_type->child_count - 1];

    status = primitive_from_string(typeKind->value, &composite->primitive);

    // type kind is not primitve, must be a predefined composite

    if (status == SEMANTIC_ERROR) {
        // not a primitive try to resolve the type by name (must be a composite)
        Type *nested_type = NULL;
        status = get_type_decl(typeKind->value, &nested_type);

        if (status == SEMANTIC_ERROR) {
            print_diagnostic(current_file, &typeKind->location, Error, "Unknown composite type in declaration");
            return SEMANTIC_ERROR;
        }

        if (nested_type->kind == TypeKindComposite) {
            // valid composite type

            composite->primitive = nested_type->impl.composite.primitive;

            // no sign was set, use sign of type
            if (scaleNodeOffset == 0) {
                composite->sign = nested_type->impl.composite.sign;
            }

            composite->scale = composite->scale * nested_type->impl.composite.scale;

            if(composite->scale > 8 || composite->scale < 0.25) {
                return  SEMANTIC_ERROR;
            }

        } else {
            print_diagnostic(current_file, &typeKind->location, Error, "Type must be either composite or primitive");
            return SEMANTIC_ERROR;
        }
    }

    return check_scale_factor(ast_type, composite->scale);
}

/**
 * @brief Converts the given AST node to a gemstone type implementation.
 * @param currentNode AST node of type kind type
 * @param type pointer output for the type
 * @return the gemstone type implementation
 */
int get_type_impl(AST_NODE_PTR currentNode, Type **type) {
    assert(currentNode != NULL);
    assert(currentNode->kind == AST_Type);
    assert(currentNode->child_count > 0);
    DEBUG("start Type");

    int status;

    const char *typekind = currentNode->children[currentNode->child_count - 1]->value;

    //find type in composites
    if (g_hash_table_contains(declaredComposites, typekind) == TRUE) {
        *type = g_hash_table_lookup(declaredComposites, typekind);
        return SEMANTIC_OK;



    }

    if (g_hash_table_contains(declaredBoxes, typekind) == TRUE) {
        *type = g_hash_table_lookup(declaredBoxes, typekind);
        if(currentNode->child_count > 1) {

            return SEMANTIC_ERROR;
        }
        return SEMANTIC_OK;
    }
    // type is not yet declared, make a new one

    Type *new_type = mem_alloc(MemoryNamespaceSet, sizeof(Type));
    new_type->nodePtr = currentNode;

    // only one child means either composite or primitive
    // try to implement primitive first
    // if not successfull continue building a composite
    if (currentNode->child_count == 1) {
        // type is a primitive
        new_type->kind = TypeKindPrimitive;

        status = primitive_from_string(typekind, &new_type->impl.primitive);

        // if err continue at composite construction
        if (status == SEMANTIC_OK) {
            *type = new_type;
            return SEMANTIC_OK;
        }
        return SEMANTIC_ERROR;
    }

    new_type->kind = TypeKindComposite;
    new_type->impl.composite.nodePtr = currentNode;
    status = impl_composite_type(currentNode, &new_type->impl.composite);
    *type = new_type;

    return status;
}

StorageQualifier Qualifier_from_string(const char *str) {
    assert(str != NULL);

    if (strcmp(str, "local") == 0)
        return Local;
    if (strcmp(str, "static") == 0)
        return Static;
    if (strcmp(str, "global") == 0)
        return Global;

    PANIC("Provided string is not a storagequalifier: %s", str);
}

int addVarToScope(Variable *variable);

int createRef(AST_NODE_PTR currentNode, Type** reftype) {
    assert(currentNode != NULL);
    assert(currentNode->child_count == 1);
    assert(AST_get_node(currentNode,0)->kind == AST_Type);



    Type * type = malloc(sizeof(Type));
    Type * referenceType = malloc(sizeof(Type));
    referenceType->kind = TypeKindReference;
    referenceType->nodePtr = currentNode;


    int signal = get_type_impl(currentNode->children[0],&type);
    if(signal) {
        return SEMANTIC_ERROR;
    }
    referenceType->impl.reference = type;
    *reftype = referenceType;
    return SEMANTIC_OK;

}

int createDecl(AST_NODE_PTR currentNode, GArray **variables) {
    DEBUG("create declaration");

    AST_NODE_PTR ident_list = currentNode->children[currentNode->child_count - 1];

    *variables = mem_new_g_array(MemoryNamespaceSet, sizeof(Variable *));

    VariableDeclaration decl;
    decl.nodePtr = currentNode;
    decl.qualifier = Static;

    int status = SEMANTIC_OK;

    DEBUG("Child Count: %i", currentNode->child_count);

    for (size_t i = 0; i < currentNode->child_count; i++) {
        switch (currentNode->children[i]->kind) {
            case AST_Storage:
                DEBUG("fill Qualifier");
                decl.qualifier = Qualifier_from_string(AST_get_node(currentNode,i)->value);
                break;
            case AST_Type:
                DEBUG("fill Type");
                status = get_type_impl(AST_get_node(currentNode,i), &decl.type);
                break;
            case AST_IdentList:
                break;
            case AST_Reference:
                status = createRef(AST_get_node(currentNode,i), &decl.type);
                break;
            default:
                PANIC("invalid node type: %ld", currentNode->children[i]->kind);
                break;
        }
    }

    for (size_t i = 0; i < ident_list->child_count; i++) {
        Variable *variable = mem_alloc(MemoryNamespaceSet, sizeof(Variable));

        variable->kind = VariableKindDeclaration;
        variable->nodePtr = currentNode;
        variable->name = ident_list->children[i]->value;
        variable->impl.declaration = decl;

        g_array_append_val(*variables, variable);
        int signal = addVarToScope(variable);
        if (signal) {
            return SEMANTIC_ERROR;
        }
    }

    return status;
}

Expression *createExpression(AST_NODE_PTR currentNode);

int createDef(AST_NODE_PTR currentNode, GArray **variables) {
    assert(variables != NULL);
    assert(currentNode != NULL);
    assert(currentNode->kind == AST_Def);

    DEBUG("create definition");

    AST_NODE_PTR declaration = currentNode->children[0];
    AST_NODE_PTR expression = currentNode->children[1];
    AST_NODE_PTR ident_list = declaration->children[currentNode->child_count - 1];

    *variables = mem_new_g_array(MemoryNamespaceSet, sizeof(Variable *));

    VariableDeclaration decl;
    VariableDefiniton def;
    def.nodePtr = currentNode;
    decl.qualifier = Static;
    decl.nodePtr = AST_get_node(currentNode, 0);

    int status = SEMANTIC_OK;

    DEBUG("Child Count: %i", declaration->child_count);
    for (size_t i = 0; i < declaration->child_count; i++) {
        switch (declaration->children[i]->kind) {
            case AST_Storage:
                DEBUG("fill Qualifier");
                decl.qualifier = Qualifier_from_string(declaration->children[i]->value);
                break;
            case AST_Type:
                DEBUG("fill Type");
                status = get_type_impl(declaration->children[i], &decl.type);
                break;
            case AST_IdentList:
                break;
            default:
                PANIC("invalid node type: %ld", declaration->children[i]->kind);
                break;
        }
    }

    def.declaration = decl;
    Expression *name = createExpression(expression);
    if (name == NULL) {
        status = SEMANTIC_OK;
    }
    def.initializer = name;

    for (size_t i = 0; i < ident_list->child_count; i++) {
        Variable *variable = mem_alloc(MemoryNamespaceSet, sizeof(Variable));

        variable->kind = VariableKindDefinition;
        variable->nodePtr = currentNode;
        variable->name = ident_list->children[i]->value;
        variable->impl.definiton = def;

        g_array_append_val(*variables, variable);

        if (addVarToScope(variable) == SEMANTIC_ERROR) {
            return SEMANTIC_ERROR;
        }
    }

    return status;
}

int getVariableFromScope(const char *name, Variable **variable) {
    assert(name != NULL);
    assert(variable != NULL);
    assert(Scope != NULL);
    DEBUG("getting var from scope");
    int found = 0;

    // loop through all variable scope and find a variable
    if(functionParameter != NULL) {
        if(g_hash_table_contains(functionParameter, name)) {
            *variable = g_hash_table_lookup(functionParameter, name);
            found += 1;
        }
    }
    for(size_t i = 0; i < Scope->len; i++) {


        GHashTable* variable_table = g_array_index(Scope,GHashTable* ,i );
        
        if(g_hash_table_contains(variable_table, name)) {
            if(found == 0){
                 *variable = g_hash_table_lookup(variable_table, name);
            }           
            found += 1;
        }
    }
    if (found == 1){
        DEBUG("Var: %s",(*variable)->name);
        DEBUG("Var Typekind: %d", (*variable)->kind);
        DEBUG("Found var");
      return SEMANTIC_OK;  
    }else if (found > 1) {
        WARN("Variable %s is a parameter and a declared variable. Returning parameter", name);
     return SEMANTIC_OK;  
    }
    DEBUG("nothing found");
    return SEMANTIC_ERROR;
}

int addVarToScope(Variable * variable){
    Variable* tmp = NULL;
    if(getVariableFromScope(variable->name, &tmp) == SEMANTIC_OK) {
        INFO("this var already exist: ", variable->name);
        return SEMANTIC_ERROR;
    }
    GHashTable * currentScope = g_array_index(Scope,GHashTable* ,Scope->len -1);
    g_hash_table_insert(currentScope, (gpointer) variable->name, variable);

    return SEMANTIC_OK;
}

int fillTablesWithVars(GHashTable *variableTable, const GArray* variables) {
    DEBUG("filling vars in scope and table");

    for(size_t i = 0; i < variables->len; i++) {

    
        Variable* var = g_array_index(variables,Variable *,i);

        // this variable is discarded, only need status code
        if(g_hash_table_contains(variableTable, (gpointer)var->name)){
            return SEMANTIC_ERROR;
        }

        g_hash_table_insert(variableTable, (gpointer) var->name, var);   
       }
    
    return SEMANTIC_OK;
}

[[nodiscard("type must be freed")]]
TypeValue createTypeValue(AST_NODE_PTR currentNode){
    DEBUG("create TypeValue");
    TypeValue value;
    Type *type = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    value.type = type;
    type->kind = TypeKindPrimitive;
    type->nodePtr = currentNode;

    switch (currentNode->kind) {
        case AST_Int:
            type->impl.primitive = Int;
            break;

        case AST_Float:
            type->impl.primitive = Float;
            break;

        default:
            PANIC("Node is not an expression but from kind: %i", currentNode->kind);
            break;
    }
    
    value.nodePtr = currentNode;
    value.value = currentNode->value;
    return value;
}

#define CLONE(x) mem_clone(MemoryNamespaceSet, (void*)&(x), sizeof(x))

TypeValue createString(AST_NODE_PTR currentNode) {
    DEBUG("create String");
    TypeValue value;
    Type *type = CLONE(StringLiteralType);
    value.type = type;
    value.nodePtr = currentNode;
    value.value = currentNode->value; 
    return value;
}

Type* createTypeFromOperands(Type* LeftOperandType, Type* RightOperandType, AST_NODE_PTR currentNode) {
    DEBUG("create type from operands");
    Type *result = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    result->nodePtr = currentNode;
    DEBUG("LeftOperandType->kind: %i", LeftOperandType->kind);
    DEBUG("RightOperandType->kind: %i", RightOperandType->kind);
    
    if (LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindComposite) {
        result->kind = TypeKindComposite;
        CompositeType resultImpl;

        resultImpl.nodePtr = currentNode;
        resultImpl.sign = MAX(LeftOperandType->impl.composite.sign, RightOperandType->impl.composite.sign);
        resultImpl.scale = MAX(LeftOperandType->impl.composite.scale, RightOperandType->impl.composite.scale);
        resultImpl.primitive = MAX(LeftOperandType->impl.composite.primitive , RightOperandType->impl.composite.primitive);

        result->impl.composite = resultImpl;
        
    } else if (LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindPrimitive) {
        DEBUG("both operands are primitive");
        result->kind = TypeKindPrimitive;
        
        result->impl.primitive = MAX(LeftOperandType->impl.primitive , RightOperandType->impl.primitive);

    } else if (LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindComposite) {
        result->kind = TypeKindComposite;

        result->impl.composite.sign = Signed;
        result->impl.composite.scale = MAX(1.0, RightOperandType->impl.composite.scale);
        result->impl.composite.primitive = MAX(Int, RightOperandType->impl.composite.primitive);
        result->impl.composite.nodePtr = currentNode;

    } else if (LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindPrimitive) {
        result->kind = TypeKindComposite;

        result->impl.composite.sign = Signed;
        result->impl.composite.scale = MAX(1.0, LeftOperandType->impl.composite.scale);
        result->impl.composite.primitive = MAX(Int, LeftOperandType->impl.composite.primitive);
        result->impl.composite.nodePtr = currentNode;
    } else {
        mem_free(result);
        return NULL;
    }
    DEBUG("Succsessfully created type");
    return result;
}

int createArithOperation(Expression* ParentExpression, AST_NODE_PTR currentNode, [[maybe_unused]] size_t expectedChildCount) {
    DEBUG("create arithmetic operation");
    ParentExpression->impl.operation.kind = Arithmetic;
    ParentExpression->impl.operation.nodePtr = currentNode;
    ParentExpression->impl.operation.operands = g_array_new(FALSE, FALSE,sizeof(Expression*));

    assert(expectedChildCount == currentNode->child_count);

    for (size_t i = 0; i < currentNode->child_count; i++) {
        Expression* expression = createExpression(currentNode->children[i]);

        if(NULL == expression) {
            return SEMANTIC_ERROR;
        }

        g_array_append_val(ParentExpression->impl.operation.operands, expression);
    }
    DEBUG("created all Expressions");
    switch (currentNode->kind) {
        case AST_Add:
            ParentExpression->impl.operation.impl.arithmetic = Add;
            break;
        case AST_Sub:
            ParentExpression->impl.operation.impl.arithmetic = Sub;
            break;
        case AST_Mul:
            ParentExpression->impl.operation.impl.arithmetic = Mul;
            break;
        case AST_Div:
            ParentExpression->impl.operation.impl.arithmetic = Div;
            break;
        case AST_Negate:
            ParentExpression->impl.operation.impl.arithmetic = Negate;
            break;
        default:
            PANIC("Current node is not an arithmetic operater");
            break;
    }

    if (ParentExpression->impl.operation.impl.arithmetic == Negate) {
        
        Type* result = g_array_index(ParentExpression->impl.operation.operands,Expression *,0)->result;
        result->nodePtr = currentNode;
        
        if (result->kind == TypeKindReference || result->kind == TypeKindBox) {
            print_diagnostic(current_file, &currentNode->location, Error, "Invalid type for arithmetic operation");
            return SEMANTIC_ERROR;
        } else if(result->kind == TypeKindComposite) {
            result->impl.composite.sign = Signed;
        }
        ParentExpression->result = result;
        
    } else {
        Type* LeftOperandType = g_array_index(ParentExpression->impl.operation.operands,Expression *,0)->result;
        Type* RightOperandType = g_array_index(ParentExpression->impl.operation.operands,Expression *,1)->result;

        ParentExpression->result = createTypeFromOperands(LeftOperandType, RightOperandType, currentNode);
    }

    if (ParentExpression->result == NULL) {
        return SEMANTIC_ERROR;
    }

    return SEMANTIC_OK;
}

int createRelationalOperation(Expression* ParentExpression, AST_NODE_PTR currentNode) {
    // fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Relational;
    ParentExpression->impl.operation.nodePtr = currentNode;
    ParentExpression->impl.operation.operands = g_array_new(FALSE,FALSE,sizeof(Expression*));
    // fill Operands
    for (size_t i = 0; i < currentNode->child_count; i++) {
        Expression* expression = createExpression(currentNode->children[i]);
        if(NULL == expression){
            return SEMANTIC_ERROR;
        }
        g_array_append_val(ParentExpression->impl.operation.operands, expression);
    }

    // fill impl
    switch (currentNode->kind) {
        case AST_Eq:
            ParentExpression->impl.operation.impl.relational = Equal;
            break;
        case AST_Less:
            ParentExpression->impl.operation.impl.relational = Greater;
            break;
        case AST_Greater:
            ParentExpression->impl.operation.impl.relational= Less;
            break;
        default:
            PANIC("Current node is not an relational operater");
            break;
    }

    Type* result = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    result->impl.primitive = Int;
    result->kind = TypeKindPrimitive;
    result->nodePtr = currentNode;

    ParentExpression->result = result;
    return 0;
}

int createBoolOperation(Expression *ParentExpression, AST_NODE_PTR currentNode) {
    // fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Boolean;
    ParentExpression->impl.operation.nodePtr = currentNode;

    // fill Operands
    for (size_t i = 0; i < currentNode->child_count; i++){
        Expression* expression = createExpression(currentNode->children[i]);
        if (NULL == expression) {
            return SEMANTIC_ERROR;
        }
        g_array_append_val(ParentExpression->impl.operation.operands, expression);
    }

    switch (currentNode->kind) {
        case AST_BoolAnd:
            ParentExpression->impl.operation.impl.boolean = BooleanAnd;
            break;
        case AST_BoolOr:
            ParentExpression->impl.operation.impl.boolean = BooleanOr;
            break;
        case AST_BoolXor:
            ParentExpression->impl.operation.impl.boolean = BooleanXor;
            break;
        default:
            PANIC("Current node is not an boolean operater");
            break;
    }

    Expression* lhs = ((Expression**) ParentExpression->impl.operation.operands->data)[0];
    Expression* rhs = ((Expression**) ParentExpression->impl.operation.operands->data)[1];

    Type* LeftOperandType = lhs->result;
    Type* RightOperandType = rhs->result;

    // should not be a box or a reference
    if(LeftOperandType->kind != TypeKindPrimitive && LeftOperandType->kind != TypeKindComposite) {
        print_diagnostic(current_file, &lhs->nodePtr->location, Error, "invalid type for boolean operation");
        return SEMANTIC_ERROR;
    }
    if(RightOperandType->kind != TypeKindPrimitive && RightOperandType->kind != TypeKindComposite) {
        print_diagnostic(current_file, &rhs->nodePtr->location, Error, "invalid type for boolean operation");
        return SEMANTIC_ERROR;
    }
    // should not be a float
    if (LeftOperandType->kind == TypeKindComposite) {
        if (LeftOperandType->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &lhs->nodePtr->location, Error, "operand must not be a float");
            return SEMANTIC_ERROR;
        }
    } else if (LeftOperandType->kind == TypeKindPrimitive) {
        if (LeftOperandType->impl.primitive == Float) {
            print_diagnostic(current_file, &lhs->nodePtr->location, Error, "operand must not be a float");
            return SEMANTIC_ERROR;
        }
    } else if (RightOperandType->kind == TypeKindComposite) {
        if (RightOperandType->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &rhs->nodePtr->location, Error, "operand must not be a float");
            return SEMANTIC_ERROR;
        }
    } else if (RightOperandType->kind == TypeKindPrimitive) {
        if (RightOperandType->impl.primitive == Float) {
            print_diagnostic(current_file, &rhs->nodePtr->location, Error, "operand must not be a float");
            return SEMANTIC_ERROR;
        }
    }

    ParentExpression->result = createTypeFromOperands(LeftOperandType, RightOperandType, currentNode);

    return SEMANTIC_OK;
}

int createBoolNotOperation(Expression *ParentExpression, AST_NODE_PTR currentNode) {
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Boolean;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operand
    Expression* expression = createExpression(currentNode->children[0]);
    if(NULL == expression){
        return SEMANTIC_ERROR;
    }
    g_array_append_val(ParentExpression->impl.operation.operands , expression);

    ParentExpression->impl.operation.impl.boolean = BooleanNot;

    Type* Operand = ((Expression**)ParentExpression->impl.operation.operands)[0]->result;

    Type* result = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    result->nodePtr = currentNode;

    if (Operand->kind == TypeKindBox || Operand->kind == TypeKindReference) {
        print_diagnostic(current_file, &Operand->nodePtr->location, Error, "Operand must be a variant of primitive type int");
        return SEMANTIC_ERROR;
    }

    if (Operand->kind == TypeKindPrimitive) {
        if (Operand->impl.primitive == Float) {
            print_diagnostic(current_file, &Operand->nodePtr->location, Error, "Operand must be a variant of primitive type int");
            return SEMANTIC_ERROR;
        }
        result->kind = Operand->kind;
        result->impl = Operand->impl;
        
    } else if(Operand->kind == TypeKindComposite) {
        if (Operand->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &Operand->nodePtr->location, Error, "Operand must be a variant of primitive type int");
            return SEMANTIC_ERROR;
        }
        result->kind = Operand->kind;
        result->impl = Operand->impl;      
    }

    ParentExpression->result = result;
    return SEMANTIC_OK;
}

bool isScaleEqual(double leftScale, double rightScale) {
    int leftIntScale = (int) (leftScale * BASE_BYTES);
    int rightIntScale = (int) (rightScale * BASE_BYTES);
    
    return leftIntScale == rightIntScale;
}

int createBitOperation(Expression* ParentExpression, AST_NODE_PTR currentNode) {
    // fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Boolean;
    ParentExpression->impl.operation.nodePtr = currentNode;

    // fill Operands
    for (size_t i = 0; i < currentNode->child_count; i++) {
        Expression* expression = createExpression(currentNode->children[i]);

        if(NULL == expression) {
            return SEMANTIC_ERROR;
        }

        g_array_append_val(ParentExpression->impl.operation.operands , expression);
    }

    switch (currentNode->kind) {
        case AST_BitAnd:
            ParentExpression->impl.operation.impl.bitwise = BitwiseAnd;
            break;
        case AST_BitOr:
            ParentExpression->impl.operation.impl.bitwise = BitwiseOr;
            break;
        case AST_BitXor:
            ParentExpression->impl.operation.impl.bitwise = BitwiseXor;
            break;
        default:
            PANIC("Current node is not an bitwise operater");
            break;
    }

    Type *result = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    result->nodePtr = currentNode;

    Expression* lhs = ((Expression**) ParentExpression->impl.operation.operands->data)[0];
    Expression* rhs = ((Expression**) ParentExpression->impl.operation.operands->data)[1];
    
    Type* LeftOperandType = lhs->result;
    Type* RightOperandType = rhs->result;

    //should not be a box or a reference
    if (LeftOperandType->kind != TypeKindPrimitive && LeftOperandType->kind != TypeKindComposite) {
        print_diagnostic(current_file, &lhs->nodePtr->location, Error, "Must be a type variant of int");
        return SEMANTIC_ERROR;
    }

    if (RightOperandType->kind != TypeKindPrimitive && RightOperandType->kind != TypeKindComposite) {
        print_diagnostic(current_file, &rhs->nodePtr->location, Error, "Must be a type variant of int");
        return SEMANTIC_ERROR;
    }

    if (LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindPrimitive) {

        if (LeftOperandType->impl.primitive == Float) {
            print_diagnostic(current_file, &lhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        if (RightOperandType->impl.primitive == Float) {
            print_diagnostic(current_file, &rhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;

    } else if (LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindComposite) {

        if (LeftOperandType->impl.primitive == Float) {
            print_diagnostic(current_file, &lhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        if (RightOperandType->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &rhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;

    }else if (LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindPrimitive) {

        if (LeftOperandType->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &lhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        if (RightOperandType->impl.primitive == Float) {
            print_diagnostic(current_file, &rhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;
    } else {

        if (RightOperandType->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &rhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        if (LeftOperandType->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &lhs->nodePtr->location, Error, "Must be a type variant of int");
            return SEMANTIC_ERROR;
        }

        if (!isScaleEqual(LeftOperandType->impl.composite.scale,  RightOperandType->impl.composite.scale)) {
            print_diagnostic(current_file, &currentNode->location, Error, "Operands must be of equal size");
            return SEMANTIC_ERROR;
        }

        result->kind = TypeKindComposite;
        result->impl.composite.nodePtr = currentNode;
        result->impl.composite.scale = LeftOperandType->impl.composite.scale;
        result->impl.composite.sign = MAX(LeftOperandType->impl.composite.sign, RightOperandType->impl.composite.sign);
    }

    ParentExpression->result = result;
    return 0;
}

int createBitNotOperation(Expression* ParentExpression, AST_NODE_PTR currentNode) {
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Bitwise;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operand
    Expression* expression = createExpression(currentNode->children[0]);
    if(NULL == expression){
        return SEMANTIC_ERROR;
    }
    g_array_append_val(ParentExpression->impl.operation.operands , expression);

    ParentExpression->impl.operation.impl.bitwise = BitwiseNot;

    Type* Operand = ((Expression**) ParentExpression->impl.operation.operands)[0]->result;
    
    Type* result = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    result->nodePtr = currentNode;
    
    if (Operand->kind  == TypeKindPrimitive) {

        if (Operand->impl.primitive == Float) {
            print_diagnostic(current_file, &Operand->nodePtr->location, Error, "Operand type must be a variant of int");
            return SEMANTIC_ERROR;
        }

        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;
    }else if(Operand->kind == TypeKindComposite) {

        if (Operand->impl.composite.primitive == Float) {
            print_diagnostic(current_file, &Operand->nodePtr->location, Error, "Operand type must be a variant of int");
            return SEMANTIC_ERROR;
        }

        result->kind = TypeKindComposite;
        result->impl.composite.nodePtr = currentNode;
        result->impl.composite.primitive = Int;
        result->impl.composite.sign = Operand->impl.composite.sign;
        result->impl.composite.scale = Operand->impl.composite.scale;
    }   
    
    ParentExpression->result = result;

    return SEMANTIC_OK;
}

/**
 * @brief Return a copy of all BoxMembers specified by their name in names from a boxes type
 *        Will run recursively in case the first name refers to a subbox
 * @param currentBoxType
 * @param names
 * @return
 */
GArray *getBoxMember(Type *currentBoxType, GArray *names) {

    GArray *members = mem_new_g_array(MemoryNamespaceSet, sizeof(BoxMember));
    // list of members of the type
    GHashTable *memberList = currentBoxType->impl.box->member;

    // name of member to extract
    const char *currentName = g_array_index(names, const char *, 0);
    // look for member of this name
    if (g_hash_table_contains(memberList, currentName)) {

        // get member and store in array
        BoxMember *currentMember = g_hash_table_lookup(memberList, currentName);
        g_array_append_val(members, currentMember);

        // last name in list, return
        g_array_remove_index(names, 0);
        if (names->len == 0) {
            return members;
        }

        // other names may refer to members of child boxes
        if (currentMember->type->kind == TypeKindBox) {
            GArray *otherMember = getBoxMember(currentMember->type, names);

            if (NULL == otherMember) {
                return NULL;
            }

            g_array_append_vals(members, (BoxMember *) otherMember->data, otherMember->len);

            return members;
        }
    }

    return NULL;
}

int createBoxAccess(Expression* ParentExpression,AST_NODE_PTR currentNode) {

    const char* boxname = currentNode->children[0]->value;
    Variable* boxVariable = NULL;
    int status = getVariableFromScope(boxname, &boxVariable);

    if(status == SEMANTIC_ERROR){
        print_diagnostic(current_file, &currentNode->children[0]->location, Error, "Variable of name `%s` does not exist");
        return SEMANTIC_ERROR;
    }
    Type* boxType;

    if(boxVariable->kind == VariableKindDeclaration){
        
        boxType = boxVariable->impl.declaration.type;
    } else if (boxVariable->kind == VariableKindDefinition){
        boxType = boxVariable->impl.definiton.declaration.type;
    } else{
        return SEMANTIC_ERROR;
    }
    if (boxType->kind != TypeKindBox) {
        return SEMANTIC_ERROR;
    }
    
    // filling boxAccess variable
    ParentExpression->impl.variable->kind = VariableKindBoxMember;
    ParentExpression->impl.variable->nodePtr = currentNode;
    ParentExpression->impl.variable->name = NULL;
    ParentExpression->impl.variable->impl.member.nodePtr = currentNode;

    //filling boxacces.variable
    ParentExpression->impl.variable->impl.member.variable = boxVariable;

    //first one is the box itself
    GArray* names = mem_alloc(MemoryNamespaceSet,sizeof(GArray));
    if(currentNode->kind == AST_IdentList){
        for (size_t i = 1; i < currentNode->child_count; i++){
            g_array_append_val(names, currentNode->children[i]->value);
        }
    }else if(currentNode->kind == AST_List){
        for (size_t i = 1; i < currentNode->children[1]->child_count; i++){
            g_array_append_val(names, currentNode->children[1]->children[i]->value);
        }
    }else{
        PANIC("current Node is not an Access");
    }

    GArray * boxMember = getBoxMember(boxType, names);
    ParentExpression->impl.variable->impl.member.member = boxMember;
    ParentExpression->result = g_array_index(boxMember,BoxMember,boxMember->len).type;
    return SEMANTIC_OK;

}

int createTypeCast(Expression* ParentExpression, AST_NODE_PTR currentNode){
    DEBUG("create type cast");
    ParentExpression->impl.typecast.nodePtr = currentNode;
    
    ParentExpression->impl.typecast.operand = createExpression(currentNode->children[0]);
    if (ParentExpression->impl.typecast.operand == NULL){
        return SEMANTIC_ERROR;
    }

    if (ParentExpression->impl.typecast.operand->result->kind != TypeKindComposite
        && ParentExpression->impl.typecast.operand->result->kind != TypeKindPrimitive){

        return SEMANTIC_ERROR;
    }

    Type *target = mem_alloc(MemoryNamespaceSet, sizeof(Type));
    int status = get_type_impl(currentNode->children[1], &target);
    if (status) {
        print_diagnostic(current_file, &currentNode->children[1]->location, Error, "Unknown type");
        return SEMANTIC_ERROR;
    }
    ParentExpression->impl.typecast.targetType = target;
    ParentExpression->result = target;
    return SEMANTIC_OK;
}

int createTransmute(Expression* ParentExpression, AST_NODE_PTR currentNode){
    ParentExpression->impl.transmute.nodePtr = currentNode;
    ParentExpression->impl.transmute.operand = createExpression(currentNode->children[0]);
    
    if (ParentExpression->impl.transmute.operand == NULL){
        return SEMANTIC_ERROR;
    }

    Type* target = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    int status = get_type_impl(currentNode->children[1], &target);
    if (status){
        print_diagnostic(current_file, &currentNode->children[1]->location, Error, "Unknown type");
        return SEMANTIC_ERROR;
    }

    ParentExpression->impl.typecast.targetType = target;
    ParentExpression->result = target;
    
    return SEMANTIC_OK;

}

int createDeref(Expression* ParentExpression, AST_NODE_PTR currentNode) {
    assert(currentNode->child_count == 2);
    Dereference deref;
    deref.nodePtr = currentNode;
    deref.index = createExpression(AST_get_node(currentNode,0));

    //index has to be made
    if(deref.index == NULL) {
        return SEMANTIC_ERROR;
    }
    Type * indexType = deref.index->result;
    //indexType can only be a composite or a primitive
    if(indexType->kind != TypeKindComposite && indexType->kind != TypeKindPrimitive) {

        return SEMANTIC_ERROR;
    }

    //indexType can only be int
    if(indexType->kind == TypeKindPrimitive) {
        if (indexType->impl.primitive != Int) {

            return SEMANTIC_ERROR;
        }
    }
    if(indexType->kind == TypeKindComposite) {
        if (indexType->impl.composite.primitive != Int) {

            return SEMANTIC_ERROR;
        }
    }

    deref.variable = createExpression(AST_get_node(currentNode,1));

    //variable has to be made
    if(deref.index == NULL) {

        return SEMANTIC_ERROR;
    }
    //variable can only be a reference
    if(deref.variable->result->kind != TypeKindReference) {

        return SEMANTIC_ERROR;
    }

    ParentExpression->impl.dereference = deref;
    ParentExpression->result = deref.variable->result->impl.reference;
    return SEMANTIC_OK;
}

int createAddressOf(Expression* ParentExpression, AST_NODE_PTR currentNode) {
    assert(currentNode != NULL);
    assert(currentNode->child_count == 1);

    AddressOf address_of;
    address_of.node_ptr = currentNode;
    address_of.variable = createExpression(AST_get_node(currentNode,0));

    if (address_of.variable == NULL) {

        return SEMANTIC_ERROR;
    }

    Type *resultType = malloc(sizeof(Type));
    resultType->nodePtr = currentNode;
    resultType->kind = TypeKindReference;
    resultType->impl.reference = address_of.variable->result;

    ParentExpression->impl.addressOf = address_of;
    ParentExpression->result = resultType;
    return SEMANTIC_OK;
}

Expression *createExpression(AST_NODE_PTR currentNode){
    DEBUG("create Expression");
    Expression *expression = mem_alloc(MemoryNamespaceSet,sizeof(Expression));
    expression->nodePtr = currentNode;

    switch (currentNode->kind) {
        case AST_Int:
        case AST_Float:
            expression->kind = ExpressionKindConstant;
            expression->impl.constant = createTypeValue(currentNode);
            expression->result = expression->impl.constant.type;
            break;
        case AST_String:
            expression->kind = ExpressionKindConstant;
            expression->impl.constant = createString(currentNode);
            expression->result = expression->impl.constant.type;
            break;
        case AST_Ident:
            DEBUG("find var");
            expression->kind = ExpressionKindVariable;
            int status = getVariableFromScope(currentNode->value, &(expression->impl.variable));
            if (status == SEMANTIC_ERROR) {
                DEBUG("Identifier is not in current scope");
                print_diagnostic(current_file, &currentNode->location, Error, "Variable not found");
                return NULL;
            }
            switch (expression->impl.variable->kind) {
                case VariableKindDeclaration:
                    expression->result = expression->impl.variable->impl.declaration.type;
                    DEBUG("%d", expression->impl.variable->impl.declaration.type->kind);
                    break;
                case VariableKindDefinition:
                    expression->result = expression->impl.variable->impl.definiton.declaration.type;
                    break;
                default:
                    PANIC("current Variable should not be an BoxMember");
                    break;
            }
            break;
        case AST_Add:
        case AST_Sub:
        case AST_Mul:
        case AST_Div:
            expression->kind = ExpressionKindOperation;
            if (createArithOperation(expression, currentNode, 2)) {
                return NULL;
            }
            break;
        case AST_Negate:
            expression->kind = ExpressionKindOperation;
            if (createArithOperation(expression, currentNode, 1)) {
                return NULL;
            }
            break;
        case AST_Eq:
        case AST_Less:
        case AST_Greater:
            expression->kind = ExpressionKindOperation;
            if (createRelationalOperation(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_BoolAnd:
        case AST_BoolOr:
        case AST_BoolXor:
            expression->kind = ExpressionKindOperation;
            if (createBoolOperation(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_BoolNot:
            expression->kind = ExpressionKindOperation;
            if (createBoolNotOperation(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_BitAnd:
        case AST_BitOr:
        case AST_BitXor:
            expression->kind = ExpressionKindOperation;
            if (createBitOperation(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_BitNot:
            expression->kind = ExpressionKindOperation;
            if (createBitNotOperation(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_IdentList:
        case AST_List:
            expression->kind = ExpressionKindVariable;
            if (createBoxAccess(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_Typecast:
            expression->kind = ExpressionKindTypeCast;
            if (createTypeCast(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_Transmute:
            expression->kind = ExpressionKindTransmute;
            if (createTransmute(expression, currentNode)) {
                return NULL;
            }
            break;
        case AST_Dereference:
        expression->kind = ExpressionKindDereference;
        if(createDeref(expression, currentNode)) {

            return NULL;
        }
        break;
    case AST_AddressOf:
        expression->kind = ExpressionKindAddressOf;
        if(createAddressOf(expression,currentNode)) {

            return NULL;
        }
        break;
    default:
        PANIC("Node is not an expression but from kind: %i", currentNode->kind);
        break;
    }

    DEBUG("expression result typekind: %d",expression->result->kind);
    DEBUG("successfully created Expression");
    return expression;
}
 

bool compareTypes(Type * leftType, Type * rightType) {
    if (leftType->kind != rightType->kind) {
        return FALSE;
    }
    if (leftType->kind == TypeKindPrimitive) {
        if(leftType->impl.primitive != rightType->impl.primitive) {
            return FALSE;
        }
        return TRUE;
    }
    if ( leftType->kind == TypeKindComposite) {
        CompositeType leftComposite = leftType->impl.composite;
        CompositeType rightComposite = leftType->impl.composite;
        if(leftComposite.primitive != rightComposite.primitive) {
            return FALSE;
        }
        if(leftComposite.sign != rightComposite.sign) {
            return FALSE;
        }
        if(leftComposite.scale != rightComposite.scale) {
            return FALSE;
        }
        return TRUE;
    }

    if(leftType->kind == TypeKindBox) {
        if(leftType->impl.box != rightType->impl.box) {
            return FALSE;
        }
        return TRUE;
    }

    if(leftType->kind == TypeKindReference) {
        bool result = compareTypes(leftType->impl.reference, rightType->impl.reference);
        return result;
    }

    return FALSE;
}

 int createAssign(Statement* ParentStatement, AST_NODE_PTR currentNode){
    DEBUG("create Assign");
    Assignment assign;
    assign.nodePtr = currentNode;
    const char* varName = currentNode->children[0]->value;

    int status = getVariableFromScope(varName, &assign.variable);
    if(status){
        return SEMANTIC_ERROR;
    }

    assign.value = createExpression(currentNode->children[1]);
    if(assign.value == NULL){
        return SEMANTIC_ERROR;
    }

    ParentStatement->impl.assignment = assign;
    return SEMANTIC_OK;
 }
int createStatement(Block * block, AST_NODE_PTR currentNode);

int fillBlock(Block * block,AST_NODE_PTR currentNode){
    DEBUG("start filling Block");
    block->nodePtr = currentNode;
    block->statemnts = g_array_new(FALSE,FALSE,sizeof(Statement*));
    GHashTable * lowerScope = g_hash_table_new(g_str_hash,g_str_equal);
    g_array_append_val(Scope, lowerScope);


    for(size_t i = 0; i < currentNode->child_count; i++){
        int signal = createStatement(block, AST_get_node(currentNode, i));
        if(signal){
            return SEMANTIC_ERROR;
        }
    }

    g_hash_table_destroy(lowerScope);
    g_array_remove_index(Scope, Scope->len-1);
    
    DEBUG("created Block successfully");
    return SEMANTIC_OK;
}

int createWhile(Statement * ParentStatement, AST_NODE_PTR currentNode){
    assert(ParentStatement != NULL);
    assert(currentNode != NULL);
    assert(currentNode->kind == AST_While);

    While whileStruct;
    whileStruct.nodePtr = currentNode;
    whileStruct.conditon = createExpression(currentNode->children[0]);
    if(NULL == whileStruct.conditon){
        return SEMANTIC_ERROR;
    }
    AST_NODE_PTR statementList = currentNode->children[1];

    int signal = fillBlock(&whileStruct.block,statementList);
    if(signal){
        return SEMANTIC_ERROR;
    }
    ParentStatement->impl.whileLoop = whileStruct;

    return SEMANTIC_OK;
}



int createIf(Branch* Parentbranch, AST_NODE_PTR currentNode){
    If ifbranch;
    ifbranch.nodePtr = currentNode;
    
    Expression* expression = createExpression(currentNode->children[0]);
    if (NULL == expression) {
        return SEMANTIC_ERROR;
    }
    ifbranch.conditon = expression;
    int status = fillBlock(&ifbranch.block, currentNode->children[1]);
    
    if(status){
        return SEMANTIC_ERROR;
    }
    Parentbranch->ifBranch = ifbranch;
    return SEMANTIC_OK;
}

int createElse(Branch* Parentbranch, AST_NODE_PTR currentNode){
    Else elseBranch;
    elseBranch.nodePtr = currentNode;
    
    int status = fillBlock(&elseBranch.block, currentNode->children[0]);
    
    if(status){
        return SEMANTIC_ERROR;
    }
    Parentbranch->elseBranch = elseBranch;
    return SEMANTIC_OK;
}

int createElseIf(Branch* Parentbranch, AST_NODE_PTR currentNode){
    ElseIf elseIfBranch;
    elseIfBranch.nodePtr = currentNode;
    
    Expression* expression = createExpression(currentNode->children[0]);
    if (NULL == expression) {
        return SEMANTIC_ERROR;
    }
    elseIfBranch.conditon = expression;
    int status = fillBlock(&elseIfBranch.block, currentNode->children[1]);
    
    if(status){
        return SEMANTIC_ERROR;
    }
    g_array_append_val(Parentbranch->elseIfBranches,elseIfBranch);
    return SEMANTIC_OK;
}



int createBranch(Statement* ParentStatement,AST_NODE_PTR currentNode){
    Branch Branch;
    Branch.nodePtr = currentNode;
    for (size_t i = 0; i < currentNode->child_count; i++ ){
        switch (currentNode->children[i]->kind){
            case AST_If:
                if(createIf(&Branch, currentNode->children[i])){
                    return SEMANTIC_ERROR;
                }
            break;

            case AST_IfElse:
                if(createElseIf(&Branch, currentNode)){
                    return SEMANTIC_ERROR;
                }
            break;

            case AST_Else:
                if(createElse(&Branch, currentNode->children[i])){
                    return SEMANTIC_ERROR;
                }
            break;

            default:
                PANIC("current node is not part of a Branch");
                break;
        }
    }
    ParentStatement->impl.branch = Branch;
    return SEMANTIC_OK;
}

int getFunction(const char *name, Function **function);

int createfuncall(Statement *parentStatement, AST_NODE_PTR currentNode) {
    assert(currentNode != NULL);
    assert(currentNode->child_count == 2);

    AST_NODE_PTR argsListNode = AST_get_node(currentNode, 1);
    AST_NODE_PTR nameNode = AST_get_node(currentNode, 0);



    FunctionCall funcall;
    Function * fun = NULL;
    if(nameNode->kind == AST_Ident) {
        int result = getFunction(nameNode->value, &fun);
        if (result == SEMANTIC_ERROR) {
            return SEMANTIC_ERROR;
        }
    }
    if(nameNode->kind == AST_IdentList) {
        assert(nameNode->child_count > 1);

        //idents.boxname.funname()
        //only boxname and funname are needed, because the combination is unique
        const char* boxName = AST_get_node( nameNode, (nameNode->child_count - 2))->value;
        const char* funName = AST_get_node( nameNode, (nameNode->child_count - 1))->value;

        const char* name = g_strjoin("", boxName, "." , funName, NULL );

        int result = getFunction(name,&fun);
        if (result) {
            return SEMANTIC_ERROR;
        }
    }

    funcall.function = fun;
    funcall.nodePtr = currentNode;

    size_t paramCount = 0;
    if(fun->kind == FunctionDeclarationKind) {
        paramCount = fun->impl.declaration.parameter->len;
    }else if(fun->kind == FunctionDefinitionKind) {
        paramCount = fun->impl.definition.parameter->len;
    }

    size_t count = 0;
    for(size_t i  = 0; i < argsListNode->child_count; i++) {
        count += AST_get_node(argsListNode, i)->child_count;
    }

    if(count != paramCount) {
        return SEMANTIC_ERROR;
    }

    GArray * expressions = mem_new_g_array(MemoryNamespaceSet,(sizeof(Expression*)));
    //exprlists
    for( size_t i = 0; i < argsListNode->child_count; i++) {
        AST_NODE_PTR currentExprList = AST_get_node(argsListNode, i);

        for( size_t j = 0; j < currentExprList->child_count; j++) {
            Expression *expr = createExpression(AST_get_node(currentExprList, j));
            if(expr == NULL) {
                return SEMANTIC_ERROR;
            }
            g_array_append_val(expressions, expr);
        }
    }
    funcall.expressions = expressions;


    parentStatement->impl.call = funcall;
    return SEMANTIC_OK;

}


int createStatement(Block * Parentblock , AST_NODE_PTR currentNode){
    DEBUG("create Statement");
        
    switch(currentNode->kind){
        case AST_Decl:{
                GArray *variable= g_array_new(FALSE, FALSE, sizeof(Variable*));
                
                int status = createDecl(currentNode, &variable);
                if(status){
                    return SEMANTIC_ERROR;
                }
                for(size_t i = 0; i < variable->len ; i++){

                    Statement * statement = mem_alloc(MemoryNamespaceSet,sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindDeclaration;

                
                statement->impl.variable = g_array_index(variable,Variable *,i);
                g_array_append_val(Parentblock->statemnts,statement);
                }
            }
            break;

        case AST_Def:{
                GArray *variable= g_array_new(FALSE, FALSE, sizeof(Variable*));
                
                int status = createDef(currentNode, &variable);

                if(status){
                    return SEMANTIC_ERROR;
                }
                for(size_t i = 0; i < variable->len ; i++){

                Statement * statement = mem_alloc(MemoryNamespaceSet,sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindDefinition;
                
                statement->impl.variable = g_array_index(variable,Variable *,i);
                g_array_append_val(Parentblock->statemnts,statement);
                }
                
            }
            break;
        case AST_While:{
                Statement * statement = mem_alloc(MemoryNamespaceSet,sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindWhile;
                if(createWhile(statement, currentNode)){
                    return SEMANTIC_ERROR;
                }
                g_array_append_val(Parentblock->statemnts,statement);
            }
            break;
        case AST_Stmt:{
                Statement * statement = mem_alloc(MemoryNamespaceSet,sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindBranch;
                if(createBranch(statement, currentNode)){
                    return SEMANTIC_ERROR;
                }
                g_array_append_val(Parentblock->statemnts,statement);
            }
            break;
        case AST_Assign:{
                Statement * statement = mem_alloc(MemoryNamespaceSet,sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindAssignment;
                if(createAssign(statement, currentNode)){
                    return SEMANTIC_ERROR;
                }
                g_array_append_val(Parentblock->statemnts,statement);
                }
                break;
        case AST_Call:
            Statement * statement = mem_alloc(MemoryNamespaceSet,sizeof(Statement));
            statement->nodePtr = currentNode;
            statement->kind = StatementKindFunctionCall;
            int result = createfuncall(statement, currentNode);
            if(result == SEMANTIC_ERROR) {
                return SEMANTIC_ERROR;
            }
            break;
            default:
                PANIC("Node is not a statement");
         break;
    }
    
    return SEMANTIC_OK;
}


int createParam(GArray * Paramlist ,AST_NODE_PTR currentNode){
    assert(currentNode->kind == AST_Parameter);
    DEBUG("start param");
    DEBUG("current node child count: %i",currentNode->child_count);

    AST_NODE_PTR paramdecl = currentNode->children[1];
    AST_NODE_PTR ioQualifierList =  currentNode->children[0];

    ParameterDeclaration decl;
    decl.nodePtr = paramdecl;

    DEBUG("iolistnode child count: %i", ioQualifierList->child_count );
    if(ioQualifierList->child_count == 2){
        decl.qualifier = InOut;
    }else if(ioQualifierList->child_count == 1){
        if(strcmp(ioQualifierList->children[0]->value , "in") == 0){
            decl.qualifier = In;
        }else if(strcmp(ioQualifierList->children[0]->value , "out") == 0){
            decl.qualifier = Out;
        }else{
            PANIC("IO_Qualifier is not in or out");
        }
    }else{
        PANIC("IO_Qualifier has not the right amount of children");
    }
    
    int signal = get_type_impl(paramdecl->children[0], &(decl.type));
    if(signal){
        return SEMANTIC_ERROR;
    }
    Parameter param;
    param.nodePtr = currentNode;
    param.kind = ParameterDeclarationKind;
    param.impl.declaration = decl;
    param.name = paramdecl->children[1]->value;
    
    DEBUG("param name: %s", param.name);
    g_array_append_val(Paramlist, param);

    DEBUG("create var for param");

    Variable * paramvar = mem_alloc(MemoryNamespaceSet,sizeof(Variable));
    paramvar->kind = VariableKindDeclaration;
    paramvar->name = param.name;
    paramvar->nodePtr = currentNode;
    paramvar->impl.declaration.nodePtr = currentNode;
    paramvar->impl.declaration.qualifier = Local;
    paramvar->impl.declaration.type = param.impl.declaration.type;


    if (g_hash_table_contains(functionParameter, param.name)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(functionParameter, (gpointer)param.name, paramvar);

    DEBUG("created param successfully");
    return SEMANTIC_OK;
}


int createFunDef(Function * Parentfunction ,AST_NODE_PTR currentNode){
    DEBUG("start fundef");
    AST_NODE_PTR nameNode = currentNode->children[0];
    AST_NODE_PTR paramlistlist = currentNode->children[1];
    AST_NODE_PTR statementlist = currentNode->children[2];



    FunctionDefinition fundef;

    fundef.nodePtr = currentNode;
    fundef.name = nameNode->value;
    fundef.body = mem_alloc(MemoryNamespaceSet,sizeof(Block));
    fundef.parameter = g_array_new(FALSE, FALSE, sizeof(Parameter));

    DEBUG("paramlistlist child count: %i", paramlistlist->child_count);
    for(size_t i = 0; i < paramlistlist->child_count; i++){

        //all parameterlists
        AST_NODE_PTR paramlist = paramlistlist->children[i];
        DEBUG("paramlist child count: %i", paramlist->child_count);
        for (size_t j = 0; j < paramlist->child_count; j++){

            DEBUG("param child count: %i", AST_get_node(paramlist, j)->child_count);
            int signal = createParam(fundef.parameter ,AST_get_node(paramlist, j));
            //all params per list
           
            if (signal){
                return SEMANTIC_ERROR;
            }
        }
        DEBUG("End of Paramlist");
    }   
    int signal = fillBlock(fundef.body, statementlist);
    if(signal){
        return SEMANTIC_ERROR;
    }
    
    Parentfunction->nodePtr = currentNode;
    Parentfunction->kind = FunctionDefinitionKind;
    Parentfunction->impl.definition = fundef;
    Parentfunction->name = fundef.name;
    return SEMANTIC_OK;


}

bool compareParameter(GArray *leftParameter,GArray *rightParameter) {
    if(leftParameter->len != rightParameter->len) {
        return FALSE;
    }
    for(size_t i = 0; i < leftParameter->len; i++) {
        Parameter currentLeftParam = g_array_index(leftParameter,Parameter ,i);
        Parameter currentRightParam = g_array_index(leftParameter,Parameter ,i);
        if(strcmp(currentLeftParam.name, currentRightParam.name) !=0 ) {
            return FALSE;
        }
        if(currentLeftParam.kind != currentRightParam.kind) {
            return FALSE;
        }

        if(currentLeftParam.kind == ParameterDeclarationKind) {
            ParameterDeclaration leftDecl =  currentLeftParam.impl.declaration;
            ParameterDeclaration rightDecl =  currentLeftParam.impl.declaration;

            if(leftDecl.qualifier != rightDecl.qualifier) {
                return FALSE;
            }
            if(compareTypes(leftDecl.type, rightDecl.type) == FALSE) {
                return FALSE;
            }
        }

    }
    return TRUE;
}

int addFunction(const char *name, Function *function) {
    if(function->kind == FunctionDefinitionKind) {
        if(g_hash_table_contains(definedFunctions, name)) {
            return SEMANTIC_ERROR;
        }
        g_hash_table_insert(declaredFunctions, (gpointer) name, function);
    }else if(function->kind == FunctionDeclarationKind) {
        if(g_hash_table_contains(declaredFunctions, name)) {
            Function * declaredFunction = g_hash_table_lookup(declaredFunctions, name);
            bool result = compareParameter(declaredFunction->impl.declaration.parameter, function->impl.declaration.parameter);
            if(result == FALSE) {
                return SEMANTIC_ERROR;
            }
            //a function can have multiple declartations but all have to be identical
        }
        g_hash_table_insert(declaredFunctions,(gpointer)name, function);
    }
    return SEMANTIC_OK;

}

int getFunction(const char *name, Function **function) {
    if(g_hash_table_contains(definedFunctions, name)) {
        Function * fun = g_hash_table_lookup(definedFunctions, name);
        *function = fun;
        return SEMANTIC_OK;
    }
    if(g_hash_table_contains(declaredFunctions, name)) {
        Function * fun = g_hash_table_lookup(declaredFunctions, name);
        *function = fun;
        return SEMANTIC_OK;
    }
    return SEMANTIC_ERROR;
}

int createFunDecl(Function * Parentfunction ,AST_NODE_PTR currentNode){
    DEBUG("start fundecl");
    AST_NODE_PTR nameNode = currentNode->children[0];
    AST_NODE_PTR paramlistlist = currentNode->children[1];



    FunctionDeclaration fundecl;

    fundecl.nodePtr = currentNode;
    fundecl.name = nameNode->value;
    fundecl.parameter = mem_alloc(MemoryNamespaceSet,sizeof(GArray));

    
    for(size_t i = 0; i < paramlistlist->child_count; i++){

        //all parameterlists
        AST_NODE_PTR paramlist = paramlistlist->children[i];

        for (size_t j = 0; j < paramlistlist->child_count; j++){

            int signal = createParam(fundecl.parameter ,paramlist->children[i]);
            //all params per list
            if (signal){
                return SEMANTIC_ERROR;
            }
        }
    }   

    Parentfunction->nodePtr = currentNode;
    Parentfunction->kind = FunctionDefinitionKind;
    Parentfunction->impl.declaration = fundecl;
    Parentfunction->name = fundecl.name;
    return SEMANTIC_OK;
}


int createFunction(Function ** function, AST_NODE_PTR currentNode){
    assert(currentNode->kind == AST_Fun);
    Function * fun = mem_alloc(MemoryNamespaceSet,sizeof(Function));
    functionParameter = g_hash_table_new(g_str_hash,g_str_equal);
     
    if(currentNode->child_count == 2){
        int signal = createFunDecl(fun, currentNode);
        if (signal){
            return SEMANTIC_ERROR;
        }
    }else if(currentNode->child_count == 3){
        int signal = createFunDef(fun, currentNode);
        if (signal){
            return SEMANTIC_ERROR;
        }
    }else {
        PANIC("function should have 2 or 3 children");
    }



    g_hash_table_destroy(functionParameter);

    int result = addFunction(fun->name,fun);
    if(result == SEMANTIC_ERROR) {
        return SEMANTIC_ERROR;
    }

    *function = fun;
    return SEMANTIC_OK;
} 



int createDeclMember(BoxType * ParentBox, AST_NODE_PTR currentNode){

    Type * declType = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    int status = get_type_impl(currentNode->children[0],&declType);
    if(status){
    return SEMANTIC_ERROR;
    }

    AST_NODE_PTR nameList = currentNode->children[1];
    for(size_t i = 0; i < nameList->child_count; i++){
        BoxMember * decl = mem_alloc(MemoryNamespaceSet,sizeof(BoxMember));
        decl->name = nameList->children[i]->value;
        decl->nodePtr = currentNode;
        decl->box = ParentBox;
        decl->initalizer = NULL;
        decl->type = declType;
        if(g_hash_table_contains(ParentBox->member, (gpointer)decl->name)){
            return SEMANTIC_ERROR;
        }
        g_hash_table_insert(ParentBox->member,(gpointer)decl->name,decl);
    }
    return SEMANTIC_OK;
}

int createDefMember(BoxType *ParentBox, AST_NODE_PTR currentNode){
    AST_NODE_PTR declNode = currentNode->children[0];
    AST_NODE_PTR expressionNode = currentNode->children[1];
    AST_NODE_PTR nameList = declNode->children[1];

    Type * declType = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    int status = get_type_impl(currentNode->children[0],&declType);
    if(status){
    return SEMANTIC_ERROR;
    }
    
    Expression * init = createExpression(expressionNode);;
    if (init == NULL){
        return SEMANTIC_ERROR;
    }
    
    for (size_t i = 0; i < nameList->child_count; i++){
    BoxMember *def = mem_alloc(MemoryNamespaceSet,sizeof(BoxMember));
    def->box = ParentBox;
    def->type = declType;
    def->initalizer = init;
    def->name = nameList->children[i]->value;
    def->nodePtr = currentNode;
    if(g_hash_table_contains(ParentBox->member, (gpointer)def->name)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(ParentBox->member,(gpointer)def->name,def);
    }
    return SEMANTIC_OK;
}

int createBoxFunction(const char* boxName, Type *ParentBoxType, AST_NODE_PTR currentNode) {
    Function * function = mem_alloc(MemoryNamespaceSet, sizeof(Function));

    int result  = createFunction(&function,currentNode);
    if( result == SEMANTIC_ERROR) {
        return SEMANTIC_ERROR;
    }

    function->name = g_strjoin("", boxName , "." ,  function->name, NULL );

    Parameter param;
    param.name = "self";
    param.nodePtr = currentNode;
    param.kind = ParameterDeclarationKind;
    param.impl.declaration.qualifier = In;
    param.impl.declaration.nodePtr = currentNode;
    param.impl.declaration.type = ParentBoxType;

    if(function->kind == FunctionDeclarationKind) {
        g_array_prepend_val(function->impl.declaration.parameter,param);
    }else {
        g_array_append_val(function->impl.definition.parameter, param);
    }

    addFunction(function->name,function);
    return SEMANTIC_OK;
}

int createBox(GHashTable *boxes, AST_NODE_PTR currentNode){
    BoxType * box = mem_alloc(MemoryNamespaceSet,sizeof(BoxType));
    
    box->nodePtr = currentNode;
    const char * boxName = currentNode->children[0]->value;
    AST_NODE_PTR boxMemberList = currentNode->children[1];

    Type * boxType = mem_alloc(MemoryNamespaceSet, sizeof(Type));
    boxType->nodePtr = currentNode;
    boxType->impl.box = box;


    for (size_t i = 0; boxMemberList->child_count; i++){
        switch (boxMemberList->children[i]->kind) {
            case AST_Decl:
                if(createDeclMember(box, boxMemberList->children[i])){
                    return SEMANTIC_ERROR;
                }
                break;
            case AST_Def:
                if(createDeclMember(box, boxMemberList->children[i])){
                    return SEMANTIC_ERROR;
                }
                break;
            case AST_Fun:{
                int result = createBoxFunction(boxName,boxType,AST_get_node(boxMemberList, i));
                if (result == SEMANTIC_ERROR){
                    return SEMANTIC_ERROR;
                }
            }
            default:
                break;
        }
        
    }
    if(g_hash_table_contains(boxes, (gpointer)boxName)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(boxes, (gpointer)boxName, box);



    return SEMANTIC_OK;
    
}
    
int createTypeDef(GHashTable *types, AST_NODE_PTR currentNode){
    DEBUG("create Type define");
    AST_NODE_PTR typeNode = currentNode->children[0];
    AST_NODE_PTR nameNode = currentNode->children[1];
    
    
    Type * type = mem_alloc(MemoryNamespaceSet,sizeof(Type));
    int status = get_type_impl( typeNode, &type);
    if(status){
        return SEMANTIC_ERROR;
    }
    
    Typedefine *def = mem_alloc(MemoryNamespaceSet,sizeof(Typedefine));
    def->name = nameNode->value;
    def->nodePtr = currentNode;
    def->type = type;
    
    if(g_hash_table_contains(types, (gpointer)def->name)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(types, (gpointer)def->name, def);
    if(g_hash_table_contains(declaredComposites, (gpointer)def->name)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(declaredComposites, (gpointer)def->name, def);

    return SEMANTIC_OK;
}

Module *create_set(AST_NODE_PTR currentNode){
    DEBUG("create root Module");
    //create tables for types 
    declaredComposites = g_hash_table_new(g_str_hash,g_str_equal);
    declaredBoxes = g_hash_table_new(g_str_hash,g_str_equal);
    declaredFunctions = g_hash_table_new(g_str_hash,g_str_equal);
    definedFunctions = g_hash_table_new(g_str_hash,g_str_equal);

    //create scope
    Scope = g_array_new(FALSE, FALSE, sizeof(GHashTable*));


    //building current scope for module
    GHashTable *globalscope = g_hash_table_new(g_str_hash, g_str_equal);
    globalscope = g_hash_table_new(g_str_hash,g_str_equal);
    g_array_append_val(Scope, globalscope);

    Module *rootModule = mem_alloc(MemoryNamespaceSet,sizeof(Module));

    GHashTable *boxes = g_hash_table_new(g_str_hash,g_str_equal);
    GHashTable *types = g_hash_table_new(g_str_hash,g_str_equal);
    GHashTable *functions = g_hash_table_new(g_str_hash,g_str_equal);
    GHashTable *variables = g_hash_table_new(g_str_hash,g_str_equal);
    GArray *imports = g_array_new(FALSE, FALSE, sizeof(const char*));
    
    rootModule->boxes = boxes;
    rootModule->types = types;
    rootModule->functions = functions;
    rootModule->variables = variables;
    rootModule->imports = imports;

    DEBUG("created Module struct");


    for (size_t i = 0; i < currentNode->child_count; i++){
        DEBUG("created Child with type: %i", currentNode->children[i]->kind);
        switch (currentNode->children[i]->kind) {

            case AST_Decl: {
                GArray *vars;
                int status = createDecl(currentNode->children[i], &vars);
                if (status) {
                    return NULL;
                }
                if (fillTablesWithVars(variables, vars) == SEMANTIC_ERROR) {
                    // TODO: this diagnostic will highlight entire declaration of
                    //       of variables even if just one of the declared variables
                    //       is duplicate. Consider moving this diagnostic to
                    //       `fillTablesWithVars` for more precise messaging.
                    print_diagnostic(current_file, &currentNode->children[i]->location, Error,
                                     "Variable already declared");
                    INFO("var already exists");
                    break;
                }
                DEBUG("filled successfull the module and scope with vars");
                break;
            }
            case AST_Def: {
                GArray *vars;
                int status = createDef(currentNode->children[i], &vars);
                if (status) {
                    return NULL;
                }
                DEBUG("created Definition successfully");
                break;
            }
            case AST_Box: {
                int status = createBox(boxes, currentNode->children[i]);
                if (status) {
                return NULL;
            }

                DEBUG("created Box successfully");
                break;
            }
        case AST_Fun:{
                DEBUG("start function");
                Function * function = mem_alloc(MemoryNamespaceSet,sizeof(Function));
            int status = createFunction(&function,currentNode->children[i]);
                if(status == SEMANTIC_ERROR) {
                    return NULL;
                }
                if(g_hash_table_contains(functions,function->name)){
                    return NULL;
                }
                g_hash_table_insert(functions,(gpointer)function->name, function);
            if (status){
                return NULL;
            }
            DEBUG("created function successfully");
            break;

            }
        case AST_Typedef:{
            int status = createTypeDef(types, currentNode->children[i]);
            if (status){
                return NULL;
            }
            DEBUG("created Typedef successfully");
            break;
            }
        case AST_Import:
            DEBUG("create Import");
            g_array_append_val(imports, currentNode->children[i]->value);
            break;
        default:
            INFO("Provided source file could not be parsed because of semantic error.");
            break;

        }
    }
    DEBUG("created set successfully");
    return rootModule;
}



