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
#include <set/set.h>


extern ModuleFile * current_file;
static GHashTable *declaredComposites = NULL;//pointer to composites with names 
static GHashTable *declaredBoxes = NULL;//pointer to typeboxes
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
int sign_from_string(const char* string, Sign* sign) {
    assert(string != NULL);
    assert(sign != NULL);

    if (strcmp(string, "unsigned") == 0) {
        *sign = Unsigned;
        return 0;
    } else if (strcmp(string, "signed") == 0) {
        *sign = Signed;
        return 0;
    }

    return 1;
}

/**
 * @brief Convert a string into a primitive type
 * @return 0 on success, 1 otherwise
 */
int primitive_from_string(const char* string, PrimitiveType* primitive) {
    assert(string != NULL);
    assert(primitive != NULL);

    if (strcmp(string, "int") == 0) {
        *primitive = Int;
        return 0;
    } else if (strcmp(string, "float") == 0) {
        *primitive = Float;
        return 0;
    }

    return 1;
}

int scale_factor_from(const char* string, double* factor) {
    assert(string != NULL);
    assert(factor != NULL);

    if (strcmp(string, "half") == 0 || strcmp(string, "short") == 0) {
        *factor = 0.5;
        return SEMANTIC_OK;
    } else if (strcmp(string, "double") == 0 || strcmp(string, "long") == 0) {
        *factor = 2.0;
        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

int check_scale_factor(AST_NODE_PTR node, Scale scale) {
    assert(node != NULL);

    if (8 > scale) {
        print_diagnostic(current_file, &node->location, Error, "Composite scale overflow");
        return SEMANTIC_ERROR;
    }
    if (0.25 > scale) {
        print_diagnostic(current_file, &node->location, Error, "Composite scale underflow");
        return SEMANTIC_ERROR;
    }
    return SEMANTIC_OK;
}

int merge_scale_list(AST_NODE_PTR scale_list, Scale* scale) {
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
int get_type_decl(const char* name, Type** type) {
    assert(name != NULL);
    assert(type != NULL);

    if (g_hash_table_contains(declaredComposites, name) == TRUE) {

        *type = (Type*) g_hash_table_lookup(declaredComposites, name);

        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

int impl_composite_type(AST_NODE_PTR ast_type, CompositeType* composite) {
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
        Type* nested_type = NULL;
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
 * @return the gemstone type implementation
 */
int get_type_impl(AST_NODE_PTR currentNode, Type** type) {
    assert(currentNode != NULL);
    assert(currentNode->kind == AST_Type);
    assert(currentNode->child_count > 0);

    int status;

    const char *typekind = currentNode->children[currentNode->child_count -1]->value;

    if (g_hash_table_contains(declaredComposites, typekind) == TRUE) {
        *type = g_hash_table_lookup(declaredComposites, typekind);
        return SEMANTIC_OK;
    }

    if (g_hash_table_contains(declaredBoxes, typekind) == TRUE) {
        *type = g_hash_table_lookup(declaredBoxes, typekind);
        return SEMANTIC_OK;
    }

    // type is not yet declared, make a new one

    Type* new_type = malloc(sizeof(Type));
    new_type->nodePtr = currentNode;

    // only one child means either composite or primitive
    // try to implement primitive first
    // if not successfull continue building a composite
    if(currentNode->child_count == 1) {    
        // type is a primitive
        new_type->kind = TypeKindPrimitive;

        status = primitive_from_string(typekind, &new_type->impl.primitive);
        
        // if err continue at composite construction
        if (status == SEMANTIC_OK) {
            return SEMANTIC_OK;
        }
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
int addVarToScope(Variable * variable);

int createDecl(AST_NODE_PTR currentNode, GArray** variables) {
    DEBUG("create declaration");

    AST_NODE_PTR ident_list = currentNode->children[currentNode->child_count - 1];

    *variables = g_array_new(FALSE, FALSE, sizeof(Variable*));

    VariableDeclaration decl;
    decl.nodePtr = currentNode;

    int status = SEMANTIC_OK;

    DEBUG("Child Count: %i", currentNode->child_count);

    for (size_t i = 0; i < currentNode->child_count; i++) {
        switch(currentNode->children[i]->kind){
            case AST_Storage:
                DEBUG("fill Qualifier");
                decl.qualifier = Qualifier_from_string(currentNode->children[i]->value);
                break;
            case AST_Type:
                DEBUG("fill Type");
                status = get_type_impl(currentNode->children[i], &decl.type);
                break;
            case AST_IdentList:
                break;
            default:
                PANIC("invalid node type: %ld", currentNode->children[i]->kind);
                break;
        }
    }

    for(size_t i = 0; i < ident_list->child_count; i++) {
        Variable* variable = malloc(sizeof(Variable));

        variable->kind = VariableKindDeclaration;
        variable->nodePtr = currentNode;
        variable->name = ident_list->children[i]->value;
        variable->impl.declaration = decl;

        g_array_append_val(*variables, variable);
        int signal = addVarToScope(variable);
        if (signal){
            return SEMANTIC_ERROR;
        }
    }
    
    return status;
}

Expression* createExpression(AST_NODE_PTR currentNode);

int createDef(AST_NODE_PTR currentNode, GArray** variables) {
    assert(variables != NULL);
    assert(currentNode != NULL);
    assert(currentNode->kind == AST_Def);

    DEBUG("create definition");

    AST_NODE_PTR declaration = currentNode->children[0];
    AST_NODE_PTR expression = currentNode->children[1];
    AST_NODE_PTR ident_list = currentNode->children[0]->children[currentNode->child_count - 1];

    *variables = g_array_new(FALSE, FALSE, sizeof(Variable*));

    VariableDeclaration decl;
    VariableDefiniton def;
    def.nodePtr = currentNode;

    int status = SEMANTIC_OK;

    DEBUG("Child Count: %i", declaration->child_count);
    for (size_t i = 0; i < declaration->children[i]->child_count; i++){
        switch(declaration->children[i]->kind) {
            case AST_Storage:
                DEBUG("fill Qualifier");
                decl.qualifier = Qualifier_from_string(currentNode->children[i]->value);
                break;
            case AST_Type:
                DEBUG("fill Type");
                status = get_type_impl(currentNode->children[i], &decl.type);
                break;
            case AST_IdentList:
                break;
            default:
                PANIC("invalid node type: %ld", currentNode->children[i]->kind);
                break;
            }
    }

    def.declaration = decl;
    Expression * name = createExpression(expression);
    if (name == NULL){
        status = SEMANTIC_OK;
    }
    def.initializer = name;
    

    for(size_t i = 0; i < ident_list->child_count; i++) {
        Variable* variable = malloc(sizeof(Variable));

        variable->kind = VariableKindDefinition;
        variable->nodePtr = currentNode;
        variable->name = ident_list->children[i]->value;
        variable->impl.definiton = def;
        g_array_append_val(*variables, variable);
        int signal = addVarToScope(variable);
        if (signal){
            return SEMANTIC_ERROR;
        }
    }

    return status;
}

//int: a,b,c = 5
//
//GArray.data:
//    1. Variable
//        kind = VariableKindDefinition;
//        name = a;
//        impl.definition:
//            initilizer: 
//                  createExpression(...)
//            decl:
//                qulifier:
//                type:
//                pointer
//
//
//    2. Variable       
//        kind = VariableKindDefinition;    
//        name = b; 
//        impl.definition:
//            initilizer: 5
//            decl:
//                qulifier:
//                type:
//                pointer
//    . 
//    . 
//    . 
//




int getVariableFromScope(const char* name, Variable** variable) {
    assert(name != NULL);
    assert(variable != NULL);
    assert(Scope != NULL);

    // loop through all variable scope and find a variable
    for(size_t i = 0; i < Scope->len; i++) {


        GHashTable* variable_table = g_array_index(Scope,GHashTable* ,i );
        
        if(g_hash_table_contains(variable_table, name)) {
            *variable = g_hash_table_lookup(variable_table, name);
            return SEMANTIC_OK;
        }
    }

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

int fillTablesWithVars(GHashTable *variableTable,  GArray* variables) {
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
    TypeValue value;
    Type *type = malloc(sizeof(Type));
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

static inline void* clone(int size, void* ptr) {
    char* data = malloc(size);
    memcpy(data, ptr, size);
    return data;
}

#define CLONE(x) clone(sizeof(x), (void*)&(x))

TypeValue createString(AST_NODE_PTR currentNode) {
    TypeValue value;
    Type *type = CLONE(StringLiteralType);
    value.type = type;
    value.nodePtr = currentNode;
    value.value = currentNode->value; 
    return value;
}

Type* createTypeFromOperands(Type* LeftOperandType, Type* RightOperandType, AST_NODE_PTR currentNode) {
    Type *result = malloc(sizeof(Type));
    result->nodePtr = currentNode;
    
    if (LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindComposite) {
        result->kind = TypeKindComposite;
        CompositeType resultImpl;

        resultImpl.nodePtr = currentNode;
        resultImpl.sign = MAX(LeftOperandType->impl.composite.sign, RightOperandType->impl.composite.sign);
        resultImpl.scale = MAX(LeftOperandType->impl.composite.scale, RightOperandType->impl.composite.scale);
        resultImpl.primitive = MAX(LeftOperandType->impl.composite.primitive , RightOperandType->impl.composite.primitive);

        result->impl.composite = resultImpl;
        
    } else if (LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindPrimitive) {
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
        free(result);
        return NULL;
    }
    return result;
}

int createArithOperation(Expression* ParentExpression, AST_NODE_PTR currentNode, [[maybe_unused]] size_t expectedChildCount) {
    
    ParentExpression->impl.operation.kind = Arithmetic;
    ParentExpression->impl.operation.nodePtr = currentNode;

    assert(expectedChildCount > currentNode->child_count);

    for (size_t i = 0; i < currentNode->child_count; i++) {
        Expression* expression = createExpression(currentNode->children[i]);

        if(NULL == expression) {
            return SEMANTIC_OK;
        }

        g_array_append_val(ParentExpression->impl.operation.operands, expression);
    }

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
        Type* result = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
        result->nodePtr = currentNode;
        
        if (result->kind == TypeKindReference || result->kind == TypeKindBox) {
            print_diagnostic(current_file, &currentNode->location, Error, "Invalid type for arithmetic operation");
            return SEMANTIC_ERROR;
        } else if(result->kind == TypeKindComposite) {
            result->impl.composite.sign = Signed;
        }
        ParentExpression->result = result;
        
    } else {

        Type* LeftOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
        Type* RightOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[1]->result;

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

    Type* result = malloc(sizeof(Type));
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

    Type* result = malloc(sizeof(Type));
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

    Type *result = malloc(sizeof(Type));
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
    
    Type* result = malloc(sizeof(Type));
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

GArray* getBoxMember(Type* currentBoxType, GArray *names) {
    
    GArray *members = g_array_new(FALSE, FALSE, sizeof(BoxMember));    
    GHashTable* memberList = currentBoxType->impl.box.member;
    
    const char* currentName = g_array_index(names,const char *,0);
    if(!g_hash_table_contains(memberList, currentName)) {
        // TODO: free  members
        return NULL; 
    }
    BoxMember * currentMember = g_hash_table_lookup(memberList, currentName);
    g_array_append_val(members, currentMember);

    g_array_remove_index(names,0);
    if (names->len == 0) {
        return members;
    }
    if (currentMember->type->kind == TypeKindBox){
        GArray *otherMember = getBoxMember(currentMember->type, names);
        if(NULL == otherMember){
            return NULL;
        }
        g_array_append_vals(members,(BoxMember *) otherMember->data, otherMember->len);
        return members;
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
    GArray* names = malloc(sizeof(GArray));
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
    ParentExpression->impl.typecast.nodePtr = currentNode;
    
    ParentExpression->impl.typecast.operand = createExpression(currentNode->children[0]);
    if (ParentExpression->impl.typecast.operand == NULL){
        return SEMANTIC_ERROR;
    }

    Type* target = malloc(sizeof(Type));
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

    Type* target = malloc(sizeof(Type));
    int status = get_type_impl(currentNode->children[1], &target);
    if (status){
        print_diagnostic(current_file, &currentNode->children[1]->location, Error, "Unknown type");
        return SEMANTIC_ERROR;
    }

    ParentExpression->impl.typecast.targetType = target;
    ParentExpression->result = target;
    
    return SEMANTIC_OK;

}



Expression *createExpression(AST_NODE_PTR currentNode){
    Expression *expression = malloc(sizeof(Expression));
    expression->nodePtr = currentNode;
    switch(currentNode->kind){
    
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
        expression->kind = ExpressionKindVariable;
        int status = getVariableFromScope(currentNode->value, &expression->impl.variable  );             
        if(status == SEMANTIC_ERROR){
            DEBUG("Identifier is not in current scope");
            print_diagnostic(current_file, &currentNode->location, Error, "Variable not found");
            return NULL;
        }
        switch (expression->impl.variable->kind) {
            case VariableKindDeclaration:
                expression->result = expression->impl.variable->impl.declaration.type;
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
         if(createArithOperation(expression, currentNode, 2)){
           return NULL; 
        }
        break;
    case AST_Negate:
        expression->kind = ExpressionKindOperation;
        if(createArithOperation(expression,currentNode, 1)){
            return NULL;
        }
        break;
    case AST_Eq:
    case AST_Less:
    case AST_Greater:
        expression->kind = ExpressionKindOperation;
        if(createRelationalOperation(expression,currentNode)){
            return NULL;
        }
        break;
    case AST_BoolAnd:
    case AST_BoolOr:
    case AST_BoolXor:
        expression->kind = ExpressionKindOperation;
        if(createBoolOperation(expression,currentNode)){
            return NULL;
        }
        break;
    case AST_BoolNot:
        expression->kind= ExpressionKindOperation;
        if(createBoolNotOperation(expression, currentNode)){
            return NULL;
        }
        break;
    case AST_BitAnd:
    case AST_BitOr:
    case AST_BitXor:
        expression->kind= ExpressionKindOperation;
        if(createBitOperation(expression, currentNode)){
            return NULL;
        }
        break;
    case AST_BitNot:
        expression->kind = ExpressionKindOperation;
        if(createBitNotOperation(expression, currentNode)){
            return NULL;
        }
        break;

    case AST_IdentList:
    case AST_List:
        expression->kind = ExpressionKindVariable;
        if(createBoxAccess(expression, currentNode)){
            return NULL;
        }
        break;
    case AST_Typecast:
        expression->kind = ExpressionKindTypeCast;
        if(createTypeCast(expression, currentNode)){
            return NULL;
        }
        break;
    case AST_Transmute:
        expression->kind = ExpressionKindTransmute;
        if(createTransmute(expression, currentNode)){
            return NULL;
        }
        break;
    default:
        PANIC("Node is not an expression but from kind: %i", currentNode->kind);
        break;
    }
    return expression;
}
 

 int createAssign(Statement* ParentStatement, AST_NODE_PTR currentNode){
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
    return SEMANTIC_ERROR;
 }
int createStatement(Block * block, AST_NODE_PTR currentNode);

int fillBlock(Block * block,AST_NODE_PTR currentNode){
    block->nodePtr = currentNode;

    GHashTable * lowerScope = g_hash_table_new(g_str_hash,g_str_equal);
    g_array_append_val(Scope, lowerScope);


    for(size_t i = 0; i < currentNode->child_count; i++){
        int signal = createStatement(block, currentNode->children[i]);
        if(signal){
            return SEMANTIC_ERROR;
        }
    }

    g_hash_table_destroy(lowerScope);
    g_array_remove_index(Scope, Scope->len-1);
    
    return SEMANTIC_OK;
}

int createWhile(Statement * ParentStatement, AST_NODE_PTR currentNode){
    assert(ParentStatement == NULL);
    assert(currentNode == NULL);
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

int createStatement(Block * Parentblock , AST_NODE_PTR currentNode){

        
    switch(currentNode->kind){
        case AST_Decl:{
                GArray *variable= g_array_new(FALSE, FALSE, sizeof(Variable*));
                
                int status = createDecl(currentNode, &variable);
                if(status){
                    return SEMANTIC_ERROR;
                }
                for(size_t i = 0; i < variable->len ; i++){

                    Statement * statement = malloc(sizeof(Statement));
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

                Statement * statement = malloc(sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindDefinition;
                
                statement->impl.variable = g_array_index(variable,Variable *,i);
                g_array_append_val(Parentblock->statemnts,statement);
                }
                
            }
            break;
        case AST_While:{
                Statement * statement = malloc(sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindWhile;
                if(createWhile(statement, currentNode)){
                    return SEMANTIC_ERROR;
                }
                g_array_append_val(Parentblock->statemnts,statement);
            }
            break;
        case AST_Stmt:{
                Statement * statement = malloc(sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindBranch;
                if(createBranch(statement, currentNode)){
                    return SEMANTIC_ERROR;
                }
                g_array_append_val(Parentblock->statemnts,statement);
            }
            break;
        case AST_Assign:{
                Statement * statement = malloc(sizeof(Statement));
                statement->nodePtr = currentNode;
                statement->kind = StatementKindAssignment;
                if(createAssign(statement, currentNode)){
                    return SEMANTIC_ERROR;
                }
                g_array_append_val(Parentblock->statemnts,statement);
                }
                break;
        case AST_Call: 
            //TODO both funcall and boxfuncall
        default:
         break;
    }
    
    return SEMANTIC_OK;
}


int createParam(GArray * Paramlist ,AST_NODE_PTR currentNode){
    AST_NODE_PTR paramdecl = currentNode->children[1];
    AST_NODE_PTR ioQualifierList =  currentNode->children[0];

    ParameterDeclaration decl;
    decl.nodePtr = paramdecl;

    if(ioQualifierList->child_count == 2){
        decl.qualifier = InOut;
    }else if(ioQualifierList->child_count == 1){
        if(strcmp(ioQualifierList->children[0]->value , "in")){
            decl.qualifier = In;
        }else if(strcmp(ioQualifierList->children[0]->value , "out")){
            decl.qualifier = Out;
        }else{
            PANIC("IO_Qualifier is not in or out");
        }
    }else{
        PANIC("IO_Qualifier has not the right amount of children");
    }

    int signal = get_type_impl(paramdecl->children[0],&decl.type);
    if(signal){
        return SEMANTIC_ERROR;
    }

    
    Parameter param;
    param.nodePtr = currentNode;
    param.kind = ParameterDeclarationKind;
    param.impl.declaration = decl;
    param.name = paramdecl->children[1]->value;

    g_array_append_val(Paramlist,param);
    return SEMANTIC_OK;
}


int createFunDef(Function * Parentfunction ,AST_NODE_PTR currentNode){
    
    AST_NODE_PTR nameNode = currentNode->children[0];
    AST_NODE_PTR paramlistlist = currentNode->children[1];
    AST_NODE_PTR statementlist = currentNode->children[2];



    FunctionDefinition fundef;

    fundef.nodePtr = currentNode;
    fundef.name = nameNode->value;
    fundef.body = malloc(sizeof(Block));
    fundef.parameter = malloc(sizeof(GArray));

    int signal = fillBlock(fundef.body, statementlist);
    if(signal){
        return SEMANTIC_ERROR;
    }
    
    for(size_t i = 0; i < paramlistlist->child_count; i++){

        //all parameterlists
        AST_NODE_PTR paramlist = paramlistlist->children[i];

        for (size_t j = 0; j < paramlistlist->child_count; j++){

            int signal = createParam(fundef.parameter ,paramlist->children[i]);
            //all params per list
            if (signal){
                return SEMANTIC_ERROR;
            }
        }
    }   

    Parentfunction->nodePtr = currentNode;
    Parentfunction->kind = FunctionDefinitionKind;
    Parentfunction->impl.definition = fundef;
    return SEMANTIC_OK;


}

int createFunDecl(Function * Parentfunction ,AST_NODE_PTR currentNode){
    
    AST_NODE_PTR nameNode = currentNode->children[0];
    AST_NODE_PTR paramlistlist = currentNode->children[1];



    FunctionDeclaration fundecl;

    fundecl.nodePtr = currentNode;
    fundecl.name = nameNode->value;
    fundecl.parameter = malloc(sizeof(GArray));

    
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
    return SEMANTIC_OK;
}
//TODO check if a function is present and if a declaration is present and identical.

Function * createFunction( AST_NODE_PTR currentNode){
    Function * fun = malloc(sizeof(Function));


    if(currentNode->child_count == 2){
        int signal = createFunDecl(fun, currentNode);
        if (signal){
            return NULL;
        }
    }else if(currentNode->child_count == 3){
        int signal = createFunDef(fun, currentNode);
        if (signal){
            return NULL;
        }
    }else {
        PANIC("function should have 2 or 3 children");
    }
    
    return fun;
} 



int createDeclMember(BoxType * ParentBox, AST_NODE_PTR currentNode){

    Type * declType = malloc(sizeof(Type));
    int status = get_type_impl(currentNode->children[0],&declType);
    if(status){
    return SEMANTIC_ERROR;
    }

    AST_NODE_PTR nameList = currentNode->children[1];
    for(size_t i = 0; i < nameList->child_count; i++){
        BoxMember * decl = malloc(sizeof(BoxMember));
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

    Type * declType = malloc(sizeof(Type));
    int status = get_type_impl(currentNode->children[0],&declType);
    if(status){
    return SEMANTIC_ERROR;
    }
    
    Expression * init = createExpression(expressionNode);;
    if (init == NULL){
        return SEMANTIC_ERROR;
    }
    
    for (size_t i = 0; i < nameList->child_count; i++){
    BoxMember *def = malloc(sizeof(BoxMember));
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

int createBox(GHashTable *boxes, AST_NODE_PTR currentNode){
    BoxType * box = malloc(sizeof(BoxType));
    
    box->nodePtr = currentNode;
    const char * boxName = currentNode->children[0]->value;
    AST_NODE_PTR boxMemberList = currentNode->children[1];
    for (size_t i = 0; boxMemberList->child_count; i++){
        switch (boxMemberList->children[i]->kind) {
            case AST_Decl:
                if(createDeclMember(box, boxMemberList->children[i]->children[i])){
                    return SEMANTIC_ERROR;
                }
                break;
            case AST_Def:
                if(createDeclMember(box, boxMemberList->children[i]->children[i])){
                    return SEMANTIC_ERROR;
                }
                break;
            case AST_Fun:
            //TODO FUNCTION Wait for createFunction()
            default:
                break;
        }
        
    }
    if(g_hash_table_contains(boxes, (gpointer)boxName)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(boxes, (gpointer)boxName, box);

    return SEMANTIC_OK;

    
    //
    //box
    //  name
    //  list
    //      decl
    //      def // change BoxMember to have an 
    //      fun //create static function
    // a.b(dsadsadas)

    //type box: boxy {
    //
    //long short int: a
    //
    //short short float: floaty = 0.54
    //
    //fun main (){
    //int: a = 5
    //}
    
}
    
int createTypeDef(GHashTable *types, AST_NODE_PTR currentNode){
    AST_NODE_PTR typeNode = currentNode->children[0];
    AST_NODE_PTR nameNode = currentNode->children[1];
    
    
    Type * type = malloc(sizeof(Type));
    int status = get_type_impl( typeNode, &type);
    if(status){
        return SEMANTIC_ERROR;
    }
    
    Typedefine *def = malloc(sizeof(Typedefine));
    def->name = nameNode->value;
    def->nodePtr = currentNode;
    def->type = type;
    
    if(g_hash_table_contains(types, (gpointer)def->name)){
        return SEMANTIC_ERROR;
    }
    g_hash_table_insert(types, (gpointer)def->name, def);
    return SEMANTIC_OK;
}

Module *create_set(AST_NODE_PTR currentNode){
    DEBUG("create root Module");
    //create tables for types 
    declaredComposites = g_hash_table_new(g_str_hash,g_str_equal);
    declaredBoxes = g_hash_table_new(g_str_hash,g_str_equal);

    //create scope
    Scope = g_array_new(FALSE, FALSE, sizeof(GHashTable*));


    //building current scope for module
    GHashTable *globalscope = malloc(sizeof(GHashTable*));
    globalscope = g_hash_table_new(g_str_hash,g_str_equal);
    g_array_append_val(Scope, globalscope);

    Module *rootModule = malloc(sizeof(Module));

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
        DEBUG("created Child: %i" ,currentNode->children[i]->kind);
        switch(currentNode->children[i]->kind){
            
        case AST_Decl: {
            GArray* vars;
            int status = createDecl(currentNode->children[i], &vars);
            if (status){
                return NULL;
            }
            if (fillTablesWithVars(variables,  vars) == SEMANTIC_ERROR) {
                print_diagnostic(current_file, &currentNode->children[i]->location, Error, "Variable already declared");
                INFO("var already exists");
                break;
            }
            DEBUG("filled successfull the module and scope with vars");
            break;
        }
        case AST_Def: {
            GArray* vars;
            int status = createDef(currentNode->children[i], &vars);
            if (status){
                return NULL;
            }
            DEBUG("created Definition successfully");
            break;
        }
        case AST_Box:{
            int status = createBox(boxes, currentNode->children[i]);
            if (status){
                return NULL;
            }
            DEBUG("created Box successfully");
            break;
            }
        case AST_Fun:
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

    return rootModule;
}



