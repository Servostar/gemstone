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

GHashTable *declaredComposites = NULL;//pointer to composites with names, 
GHashTable *declaredBoxes = NULL;//pointer to typeboxes
GArray *Scope = NULL;//list of hashtables. last Hashtable is current depth of program. hashtable key: ident, value: Variable* to var

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
    if (strcmp(string, "half") == 0 || strcmp(string, "short") == 0) {
        *factor = 0.5;
        return SEMANTIC_OK;
    } else if (strcmp(string, "double") == 0 || strcmp(string, "long") == 0) {
        *factor = 2.0;
        return SEMANTIC_OK;
    }

    return SEMANTIC_ERROR;
}

int merge_scale_list(AST_NODE_PTR scale_list, Scale* scale) {
    for (size_t i = 0; i < scale_list->child_count; i++) {

        double scale_in_list = 1.0;
        int scale_invalid = scale_factor_from(AST_get_node(scale_list, i)->value, &scale_in_list);

        if (scale_invalid == SEMANTIC_ERROR) {
            return SEMANTIC_ERROR;
        }

        *scale *= scale_in_list;

        if (0.25 > *scale || 8 > *scale) {
            // TODO: print diagnostic: Invalid composite scale
            return SEMANTIC_ERROR;
        }
    }

    return SEMANTIC_OK;
}

Type *findType(AST_NODE_PTR currentNode);

int impl_composite_type(AST_NODE_PTR ast_type, CompositeType* composite) {
    DEBUG("Type is a Composite");

    int status = SEMANTIC_OK;
    int scaleNodeOffset = 0;
    
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

    const char* typeKind = ast_type->children[ast_type->child_count - 1]->value;

    status = primitive_from_string(typeKind, &composite->primitive);
    
    if (status == SEMANTIC_ERROR) {
        // not a primitive try to resolve the type by name (must be a composite)
        status = impl_composite_type();
    }

    return SEMANTIC_OK;
}

/**
 * @brief Converts the given AST node to a gemstone type implementation.
 * @param currentNode AST node of type kind type
 * @return the gemstone type implementation
 */
Type *findType(AST_NODE_PTR currentNode) {
    assert(currentNode != NULL);
    assert(currentNode->kind == AST_Type);
    assert(currentNode->child_count > 0);

    const char *typekind = currentNode->children[currentNode->child_count -1]->value;
    
    // type implementation
    Type *type = malloc(sizeof(Type));
    type->nodePtr = currentNode;

    // primitive type OR composit
    if (0 == strcmp(typekind, "int") || 0 == strcmp(typekind, "float")) {
        
        if(AST_Typekind != currentNode->children[0]->kind) {
            
            type->kind = TypeKindComposite;
            type->impl.composite.nodePtr = currentNode;
            impl_composite_type(currentNode, &type->impl.composite, typekind);
            
        } else {
            // type is a primitive
            type->kind = TypeKindPrimitive;

            int primitive_invalid = primitive_from_string(typekind, &type->impl.primitive);
            
            if (primitive_invalid) {
                PANIC("invalid primitive: %s", typekind);
            }
        }
        
    }else if(g_hash_table_contains(declaredBoxes, typekind)){
        if(AST_Typekind != currentNode->children[0]->kind){
            //TODO composite Box try
        }
        return (Type *) g_hash_table_lookup(declaredBoxes, typekind);
    }else if(g_hash_table_contains(declaredComposites, typekind)){
        if(AST_Typekind != currentNode->children[0]->kind){
            Type *composite = malloc(sizeof(Type));
            
            *composite =  *(Type*) g_hash_table_lookup(declaredComposites, typekind);
            


            size_t scalelist = 0;
            if(AST_Sign == currentNode->children[0]->kind){
                if(0 == strcmp(currentNode->children[0]->value, "unsigned")){
                    composite->impl.composite.sign = Unsigned;
                }else if(0 == strcmp(currentNode->children[0]->value, "unsigned")){
                    composite->impl.composite.sign = Signed;
                }
                scalelist = 1;
            }


            if(AST_List == currentNode->children[scalelist]->kind){
                for (size_t i = 0; i < currentNode->children[scalelist]->child_count; i++){
                    if (0 == strcmp(currentNode->children[scalelist]->children[i]->value, "short") || 0 == strcmp(currentNode->children[scalelist]->children[i]->value, "half")){
                        composite->impl.composite.scale /= 2;
                    }else{
                        composite->impl.composite.scale *= 2; 
                    }
                    if (0.25 > composite->impl.composite.scale  || 8 > composite->impl.composite.scale) {
                        //TODO scale not right
                        return NULL;
                    }
                }
            }
            return composite;
        }
        return (Type *) g_hash_table_lookup(declaredComposites, typekind);
    }else{
        //TODO doesnt know typekind 
        return NULL;
    }
    return NULL;
}



StorageQualifier Qualifier_from_string(const char *str) {
    if (!strncmp(str, "local", 5)) return Local;
    if (!strncmp(str, "static", 6)) return Static;
    if (!strncmp(str, "global", 6)) return Global;
    PANIC("Provided string is not a storagequalifier: %s", str);
}

Variable **createDecl(AST_NODE_PTR currentNode){
    DEBUG("create declaration");
    Variable **variables = malloc(currentNode->children[currentNode->child_count -1]->child_count * sizeof(Variable*));

    VariableDeclaration decl;
    decl.nodePtr = currentNode;

    DEBUG("Child Count: %i", currentNode->child_count);
    for (size_t i = 0; i < currentNode->child_count; i++){
    
        switch(currentNode->children[i]->kind){
        case AST_Storage:
            DEBUG("fill Qualifier");
            decl.qualifier = Qualifier_from_string(currentNode->children[i]->value);
            break;
        case AST_Type:
            DEBUG("fill Type");
            decl.type = findType(currentNode->children[i]);
            break;
        case AST_IdentList:
            for(size_t i = 0; i < currentNode->children[currentNode->child_count -1]->child_count; i++){
                Variable *variable = malloc(sizeof(Variable));
                variable->kind = VariableKindDeclaration;
                variable->nodePtr = currentNode;
                variable->name = currentNode->children[currentNode->child_count -1]->children[i]->value;
                variable->impl.declaration = decl;
                variables[i] = variable;
            }   
            break;
        default:
        //TODO PANIC maybe
            break;
        }
    }
    
    return variables;
}

Variable* getVariableFromScope(const char* name){
    for(size_t i = 0; i < Scope->len; i++){
        if(g_hash_table_contains(((GHashTable **) Scope->data)[i], name))
        {
            return g_hash_table_lookup(((GHashTable**)Scope->data)[i], name);
        }
    }
    return NULL;
}



int fillTablesWithVars(GHashTable *variableTable,GHashTable *currentScope , Variable** content, size_t amount){
    DEBUG("filling vars in scope and table");
    for(size_t i = 0; i < amount; i++){
        if(!(NULL == getVariableFromScope(content[i]->name))){
            DEBUG("this var already exist: ",content[i]->name);
            return 1;
        }
        g_hash_table_insert(variableTable, (gpointer) content[i]->name, content[i] );   
        g_hash_table_insert(currentScope, (gpointer) content[i]->name, content[i] );
    }
    return 0;
}





TypeValue createTypeValue(AST_NODE_PTR currentNode){
    TypeValue value;
    Type *type = malloc(sizeof(Type));
    value.type = type;
    type->kind = TypeKindPrimitive;
    type->nodePtr = currentNode;

    switch (currentNode->kind) {
   
        case AST_Int:
            type->impl.primitive = Int;
        case AST_Float:
            type->impl.primitive = Int;
        default:
        PANIC("Node is not an expression but from kind: %i", currentNode->kind);
            break;
        }
    
    value.nodePtr = currentNode;
    value.value = currentNode->value;
    return value;
}

TypeValue createString(AST_NODE_PTR currentNode){
    TypeValue value;
    Type *type =(Type*) &StringLiteralType;
    value.type = type;
    value.nodePtr = currentNode;
    value.value = currentNode->value; 
    return value;
}

Expression *createExpression(AST_NODE_PTR currentNode);


Type* createTypeFromOperands(Type* LeftOperandType, Type* RightOperandType, AST_NODE_PTR currentNode){
    Type *result = malloc(sizeof(Type));
    result->nodePtr = currentNode;

    
    if(LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindComposite)
    {
        result->kind = TypeKindComposite;
        CompositeType resultImpl;

        resultImpl.nodePtr = currentNode;
        resultImpl.sign = MAX(LeftOperandType->impl.composite.sign, RightOperandType->impl.composite.sign);
        resultImpl.scale = MAX(LeftOperandType->impl.composite.scale, RightOperandType->impl.composite.scale);
        resultImpl.primitive = MAX(LeftOperandType->impl.composite.primitive , RightOperandType->impl.composite.primitive);
        
        result->impl.composite = resultImpl;

        
    } else if(LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindPrimitive){
        result->kind = TypeKindPrimitive;
        
        result->impl.primitive = MAX(LeftOperandType->impl.primitive , RightOperandType->impl.primitive);

    } else if(LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindComposite){
        result->kind = TypeKindComposite;

        result->impl.composite.sign = Signed;
        result->impl.composite.scale = MAX( 1.0, RightOperandType->impl.composite.scale);
        result->impl.composite.primitive = MAX(Int, RightOperandType->impl.composite.primitive);
        result->impl.composite.nodePtr = currentNode;

    } else if(LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindPrimitive){
        result->kind = TypeKindComposite;

        result->impl.composite.sign = Signed;
        result->impl.composite.scale = MAX( 1.0, LeftOperandType->impl.composite.scale);
        result->impl.composite.primitive = MAX(Int, LeftOperandType->impl.composite.primitive);
        result->impl.composite.nodePtr = currentNode;
    }else{
        return NULL;
    }
    return result;
}

int createArithOperation(Expression* ParentExpression, AST_NODE_PTR currentNode, size_t expectedChildCount){
    
    ParentExpression->impl.operation.kind = Arithmetic;
    ParentExpression->impl.operation.nodePtr = currentNode;
    if (expectedChildCount > currentNode->child_count){
        PANIC("Operation has to many children");
    }
    for (size_t i = 0; i < currentNode->child_count; i++){
        Expression* expression = createExpression(currentNode->children[i]);
        if(NULL == expression){
            return 1;
        }
        g_array_append_val(ParentExpression->impl.operation.operands , expression);

    }

    switch (currentNode->kind){
        case AST_Add:
            ParentExpression->impl.operation.impl.arithmetic = Add;
        case AST_Sub:
            ParentExpression->impl.operation.impl.arithmetic = Sub;
        case AST_Mul:
            ParentExpression->impl.operation.impl.arithmetic = Mul;
        case AST_Div:
            ParentExpression->impl.operation.impl.arithmetic = Div;
        case AST_Negate:
            ParentExpression->impl.operation.impl.arithmetic = Negate;
        default:
            PANIC("Current node is not an arithmetic operater");
        break;
    }


    if(ParentExpression->impl.operation.impl.arithmetic == Negate){
        Type* result = malloc(sizeof(Type));
        result = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
        result->nodePtr = currentNode;
        if (result->kind == TypeKindReference || result->kind == TypeKindBox){
            return 1;
        }else if(result->kind == TypeKindComposite){
            result->impl.composite.sign = Signed;
        }
        ParentExpression->result = result;
        
    }else{

        Type* LeftOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
        Type* RightOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[1]->result;

        ParentExpression->result = createTypeFromOperands(LeftOperandType, RightOperandType, currentNode);
    }

    if(ParentExpression->result == NULL){
        return 1;
    }


    return 0;
}

int createRelationalOperation(Expression* ParentExpression,AST_NODE_PTR currentNode){
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Relational;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operands
    for (size_t i = 0; i < currentNode->child_count; i++){
        Expression* expression = createExpression(currentNode->children[i]);
        if(NULL == expression){
            return 1;
        }
        g_array_append_val(ParentExpression->impl.operation.operands , expression);
    }

    //fill impl
    switch (currentNode->kind){
    case AST_Eq:
        ParentExpression->impl.operation.impl.relational = Equal;
    case AST_Less:
        ParentExpression->impl.operation.impl.relational = Greater;
    case AST_Greater:
        ParentExpression->impl.operation.impl.relational= Less;
    default:
        PANIC("Current node is not an relational operater");
    break;
    }
    Type * result = malloc(sizeof(Type));
    result->impl.primitive = Int;
    result->kind = TypeKindPrimitive;
    result->nodePtr = currentNode;

    ParentExpression->result = result;
    return 0;
}


int createBoolOperation(Expression *ParentExpression, AST_NODE_PTR currentNode){
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Boolean;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operands
    for (size_t i = 0; i < currentNode->child_count; i++){
        Expression* expression = createExpression(currentNode->children[i]);
        if(NULL == expression){
            return 1;
        }
        g_array_append_val(ParentExpression->impl.operation.operands , expression);
    }

    switch (currentNode->kind){
    case AST_BoolAnd:
        ParentExpression->impl.operation.impl.boolean = BooleanAnd;
    case AST_BoolOr:
        ParentExpression->impl.operation.impl.boolean = BooleanOr;
    case AST_BoolXor:
        ParentExpression->impl.operation.impl.boolean = BooleanXor;
    default:
        PANIC("Current node is not an boolean operater");
    break;
    }


    Type* LeftOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
    Type* RightOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[1]->result;

    //should not be a box or a reference
    if(LeftOperandType->kind != TypeKindPrimitive && LeftOperandType->kind != TypeKindComposite){
        return 1;
    }
    if(RightOperandType->kind != TypeKindPrimitive && RightOperandType->kind != TypeKindComposite){
        return 1;
    }
    //should not be a float
    if(LeftOperandType->kind == TypeKindComposite){
        if(LeftOperandType->impl.composite.primitive == Float){
            return 1;
        }
    }else if(LeftOperandType->kind == TypeKindPrimitive){
        if(LeftOperandType->impl.primitive == Float){
            return 1;
        }
    }else if(RightOperandType->kind == TypeKindComposite){
        if(RightOperandType->impl.composite.primitive == Float){
            return 1;
        }
    }else if(RightOperandType->kind == TypeKindPrimitive){
        if(RightOperandType->impl.primitive == Float){
            return 1;
        }
    }


    ParentExpression->result = createTypeFromOperands(LeftOperandType, RightOperandType, currentNode);
    return 0;
}

int createBoolNotOperation(Expression *ParentExpression, AST_NODE_PTR currentNode){
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Boolean;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operand
    Expression* expression = createExpression(currentNode->children[0]);
    if(NULL == expression){
        return 1;
    }
    g_array_append_val(ParentExpression->impl.operation.operands , expression);

    ParentExpression->impl.operation.impl.boolean = BooleanNot;

    Type* Operand = ((Expression**)ParentExpression->impl.operation.operands)[0]->result;

    Type* result = malloc(sizeof(Type));
    result->nodePtr = currentNode;
    if (Operand->kind == TypeKindBox || Operand->kind == TypeKindReference){
        return 1;
    }
    if(Operand->kind == TypeKindPrimitive){
        if(Operand->impl.primitive == Float){
            return 1;
        }
        result->kind = Operand->kind;
        result->impl = Operand->impl;
    }else if(Operand->kind == TypeKindComposite){
        if(Operand->impl.composite.primitive == Float){
            return 1;
        }
        result->kind = Operand->kind;
        result->impl = Operand->impl;      
    }

    ParentExpression->result = result;
    return 0;
}

bool isScaleEqual(double leftScale, double rightScale){
    int leftIntScale =(int)(leftScale *4);
    int rightIntScale =(int)(rightScale *4);
    
    if (leftIntScale == rightIntScale){
        return TRUE;
    }
    return FALSE;
}

int createBitOperation(Expression* ParentExpression, AST_NODE_PTR currentNode){
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Boolean;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operands
    for (size_t i = 0; i < currentNode->child_count; i++){
        Expression* expression = createExpression(currentNode->children[i]);
        if(NULL == expression){
            return 1;
        }
        g_array_append_val(ParentExpression->impl.operation.operands , expression);
    }


    switch (currentNode->kind){
    case AST_BitAnd:
        ParentExpression->impl.operation.impl.bitwise = BitwiseAnd;
    case AST_BitOr:
        ParentExpression->impl.operation.impl.bitwise = BitwiseOr;
    case AST_BitXor:
        ParentExpression->impl.operation.impl.bitwise = BitwiseXor;
    default:
        PANIC("Current node is not an bitwise operater");
    break;
    }


    Type *result = malloc(sizeof(Type));
    result->nodePtr = currentNode;
    
    Type* LeftOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
    Type* RightOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[1]->result;

    //should not be a box or a reference
    if(LeftOperandType->kind != TypeKindPrimitive && LeftOperandType->kind != TypeKindComposite){
        return 1;
    }
    if(RightOperandType->kind != TypeKindPrimitive && RightOperandType->kind != TypeKindComposite){
        return 1;
    }
    if(LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindPrimitive){
        if(LeftOperandType->impl.primitive == Float || RightOperandType->impl.primitive == Float){
            return 1;
        }
        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;
    }else if(LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindComposite){
        if(LeftOperandType->impl.primitive == Float || RightOperandType->impl.composite.primitive == Float){
            return 1;
        }
        if((int)RightOperandType->impl.composite.scale != 1){
            return 1;
        }
        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;
    }else if(LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindPrimitive){
        if(LeftOperandType->impl.composite.primitive == Float || RightOperandType->impl.primitive == Float){
            return 1;
        }
        if((int)LeftOperandType->impl.composite.scale != 1){
            return 1;
        }
        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;
    }else{
        if(LeftOperandType->impl.composite.primitive == Float || RightOperandType->impl.composite.primitive == Float){
            return 1;
        }
        if(!isScaleEqual(LeftOperandType->impl.composite.scale,  RightOperandType->impl.composite.scale)){
            return 1;
        }
        result->kind = TypeKindComposite;
        result->impl.composite.nodePtr = currentNode;
        result->impl.composite.scale = LeftOperandType->impl.composite.scale;
        result->impl.composite.sign = MAX(LeftOperandType->impl.composite.sign, RightOperandType->impl.composite.sign);
    }

    ParentExpression->result = result;
    return 0;
}

int createBitNotOperation(Expression* ParentExpression, AST_NODE_PTR currentNode){
    //fill kind and Nodeptr
    ParentExpression->impl.operation.kind = Bitwise;
    ParentExpression->impl.operation.nodePtr = currentNode;

    //fill Operand
    Expression* expression = createExpression(currentNode->children[0]);
    if(NULL == expression){
        return 1;
    }
    g_array_append_val(ParentExpression->impl.operation.operands , expression);

    ParentExpression->impl.operation.impl.bitwise = BitwiseNot;

    Type* Operand = ((Expression**)ParentExpression->impl.operation.operands)[0]->result;
    
    Type* result = malloc(sizeof(Type));
    result->nodePtr = currentNode;
    
    
    if (Operand->kind  == TypeKindPrimitive){
        if(Operand->impl.primitive == Float){
            return SEMANTIC_ERROR;
        }
        result->kind = TypeKindPrimitive;
        result->impl.primitive = Int;
    }else if(Operand->kind == TypeKindComposite){
        if (Operand->impl.composite.primitive == Float){
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
        expression->impl.variable = getVariableFromScope(currentNode->value);
        if(NULL == expression->impl.variable){
            DEBUG("Identifier is not in current scope");
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
    //Box Accsess
    case AST_List:
    // Box Self Access
    case AST_Typekind:
    case AST_Transmute:
    
        

    default:
    PANIC("Node is not an expression but from kind: %i", currentNode->kind);
        break;
    }
    return expression;
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
            
        case AST_Decl:
            if (1 == fillTablesWithVars(variables,globalscope,createDecl(currentNode->children[i]) ,currentNode->children[i]->children[currentNode->children[i]->child_count -1]->child_count)){
                //TODO behandlung, wenn var schon existiert
                DEBUG("var already exists");
                break;
            }
            DEBUG("filled successfull the module and scope with vars");
            break;
        case AST_Def:
        case AST_Box:
        case AST_Fun:
        case AST_Import:
            DEBUG("create Import");
            g_array_append_val(imports, currentNode->children[i]->value);
            break;
        default:
            INFO("Provided source file could not be parsed beecause of semantic error.");
            break;

        }
    }

    return rootModule;
}



